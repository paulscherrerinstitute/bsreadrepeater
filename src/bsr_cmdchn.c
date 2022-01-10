/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_cmdchn.h>
#include <bsr_json.h>
#include <bsr_str.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum HandlerState {
    RECV = 1,
    SEND = 2,
};

void cleanup_bsr_cmdchn(struct bsr_cmdchn *k) {
    if (k->socket != NULL) {
        int ec = zmq_close(k->socket);
        if (ec != 0) {
            fprintf(stderr, "ERROR cleanup_bsr_cmdchn zmq_close %d\n", ec);
        }
        k->socket = NULL;
    }
}

ERRT bsr_cmdchn_init(struct bsr_cmdchn *self, struct bsr_poller *poller, void *user_data, char *addr,
                     struct HandlerList *handler_list, struct bsr_statistics *stats) {
    int ec;
    strncpy(self->addr, addr, ADDR_CAP - 1);
    self->addr[ADDR_CAP - 1] = 0;
    self->state = RECV;
    self->handler_list = handler_list;
    self->user_data = user_data;
    self->stats = stats;
    self->socket = zmq_socket(poller->ctx, ZMQ_REP);
    if (self->socket == NULL) {
        fprintf(stderr, "can not create socket: %s\n", zmq_strerror(errno));
        return -1;
    }
    ec = zmq_bind(self->socket, addr);
    if (ec != 0) {
        fprintf(stderr, "can not bind socket: %s\n", zmq_strerror(errno));
        return ec;
    }
    ec = zmq_poller_add(poller->poller, self->socket, user_data, ZMQ_POLLIN);
    if (ec != 0) {
        fprintf(stderr, "can not add to poller: %s\n", zmq_strerror(errno));
        return ec;
    }
    return 0;
}

ERRT bsr_cmdchn_socket_reopen_inner(struct bsr_cmdchn *self, struct bsr_poller *poller) {
    int ec;
    ec = zmq_poller_remove(poller->poller, self->socket);
    if (ec == -1) {
        fprintf(stderr, "WARN can not remove socket\n");
    }
    ec = zmq_close(self->socket);
    if (ec == -1) {
        fprintf(stderr, "WARN can not close socket\n");
    }
    self->socket = zmq_socket(poller->ctx, ZMQ_REP);
    ZMQ_NULLRET(self->socket);
    ec = zmq_bind(self->socket, self->addr);
    ZMQ_NEGONE(ec);
    ec = zmq_poller_add(poller->poller, self->socket, self->user_data, ZMQ_POLLIN);
    ZMQ_NEGONE(ec);
    return 0;
}

ERRT bsr_cmdchn_socket_reopen(struct bsr_cmdchn *self, struct bsr_poller *poller) {
    int ec = bsr_cmdchn_socket_reopen_inner(self, poller);
    if (ec != 0) {
        fprintf(stderr, "ERROR can not reopen\n");
    };
    return 0;
}

void cleanup_struct_Handler_ptr(struct Handler **k) {
    if (*k != NULL) {
        // TODO cleanup the interior as well if needed.
        free(*k);
        *k = NULL;
    }
}

void cleanup_gstring_ptr(GString **k) {
    if (*k != NULL) {
        g_string_free(*k, TRUE);
        *k = NULL;
    }
}

ERRT bsr_cmdchn_handle_event(struct bsr_cmdchn *self, struct bsr_poller *poller, struct ReceivedCommand *cmd) {
    int ec;
    cmd->ty = CmdNone;
    size_t const N = 256;
    uint8_t buf[N];
    if (self->state == RECV) {
        int do_recv = 1;
        while (do_recv) {
            int n = zmq_recv(self->socket, buf, N - 8, ZMQ_DONTWAIT);
            if (n == -1) {
                if (errno == EAGAIN) {
                    break;
                } else if (errno == EFSM) {
                    fprintf(stderr, "ERROR command recv: socket in wrong state\n");
                    bsr_cmdchn_socket_reopen(self, poller);
                    break;
                } else {
                    fprintf(stderr, "ERROR command recv %d  %s\n", errno, zmq_strerror(errno));
                    bsr_cmdchn_socket_reopen(self, poller);
                };
            } else {
                buf[n] = 0;
                fprintf(stderr, "Received command: %d [%.*s]\n", n, n, buf);
                if (n == 4 && memcmp("exit", buf, n) == 0) {
                    cmd->ty = CmdExit;
                    zmq_send(self->socket, "exit", 4, 0);
                } else if (n == 5 && memcmp("stats", buf, 5) == 0) {
                    GString *str = g_string_new("");
                    char s[256];
                    snprintf(s, sizeof(s), "received %" PRIu64 "\n", self->stats->received);
                    g_string_append(str, s);
                    snprintf(s, sizeof(s), "sent %" PRIu64 "\n", self->stats->sentok);
                    g_string_append(str, s);
                    snprintf(s, sizeof(s), "small buf %" PRIu64 "\n", self->stats->recv_buf_too_small);
                    g_string_append(str, s);
                    snprintf(s, sizeof(s), "busy %" PRIu64 "\n", self->stats->eagain);
                    g_string_append(str, s);
                    snprintf(s, sizeof(s), "busy mp %" PRIu64 "\n", self->stats->eagain_multipart);
                    g_string_append(str, s);
                    snprintf(s, sizeof(s), "poll avg %7.0f var %7.0f\n", self->stats->poll_wait_ema,
                             sqrtf(self->stats->poll_wait_emv));
                    g_string_append(str, s);
                    snprintf(s, sizeof(s), "process avg %7.0f var %7.0f\n", self->stats->process_ema,
                             sqrtf(self->stats->process_emv));
                    g_string_append(str, s);
                    zmq_send(self->socket, str->str, str->len, 0);
                    g_string_free(str, TRUE);
                } else if (n > 12 && memcmp("stats-source", buf, 12) == 0) {
                    GString *str = g_string_new("");
                    struct SplitMap sm;
                    bsr_str_split((char *)buf, n, &sm);
                    if (sm.n == 2) {
                        struct Handler *h2 = handler_list_find_by_input_addr_2(self->handler_list, sm.beg[1]);
                        if (h2 != NULL) {
                            struct bsr_chnhandler *h = &h2->handler.src;
                            char s[256];
                            snprintf(s, sizeof(s), "received %" PRIu64 "\n", h->received);
                            g_string_append(str, s);
                            snprintf(s, sizeof(s), "sent %" PRIu64 "\n", h->sentok);
                            g_string_append(str, s);
                            snprintf(s, sizeof(s), "received bytes %" PRIu64 "\n", h->recv_bytes);
                            g_string_append(str, s);
                            snprintf(s, sizeof(s), "sent bytes %" PRIu64 "\n", h->sent_bytes);
                            g_string_append(str, s);
                            snprintf(s, sizeof(s), "bsread errors %" PRIu64 "\n", h->bsread_errors);
                            g_string_append(str, s);
                            GList *it2 = h->socks_out;
                            while (it2 != NULL) {
                                struct sockout *so = it2->data;
                                snprintf(s, sizeof(s), "  Out: %s\n", so->addr);
                                g_string_append(str, s);
                                snprintf(s, sizeof(s), "    sent count %" PRIu64 "\n", so->sent_count);
                                g_string_append(str, s);
                                snprintf(s, sizeof(s), "    sent bytes %" PRIu64 "\n", so->sent_bytes);
                                g_string_append(str, s);
                                snprintf(s, sizeof(s), "    busy %" PRIu64 "\n", so->eagain);
                                g_string_append(str, s);
                                snprintf(s, sizeof(s), "    busy mp %" PRIu64 "\n", so->eagain_multipart);
                                g_string_append(str, s);
                                it2 = it2->next;
                            };
                        };
                    };
                    zmq_send(self->socket, str->str, str->len, 0);
                    g_string_free(str, TRUE);
                } else if (n >= 12 && memcmp("list-sources", buf, 12) == 0) {
                    GList *it = self->handler_list->list;
                    GString *str = g_string_new("");
                    while (it != NULL) {
                        struct Handler *handler = it->data;
                        if (handler->kind == SourceHandler) {
                            struct bsr_chnhandler *ch = &handler->handler.src;
                            g_string_append(str, ch->addr_inp);
                            g_string_append(str, "\n");
                            GList *it2 = ch->socks_out;
                            while (it2 != NULL) {
                                struct sockout *so = it2->data;
                                g_string_append(str, " Out: ");
                                g_string_append(str, so->addr);
                                g_string_append(str, "\n");
                                it2 = it2->next;
                            };
                        };
                        it = it->next;
                    };
                    zmq_send(self->socket, str->str, str->len, 0);
                    g_string_free(str, TRUE);
                } else if (n >= 11 && memcmp("add-source,", buf, 11) == 0) {
                    struct SplitMap sm;
                    bsr_str_split((char *)buf, n, &sm);
                    if (sm.n != 2) {
                        fprintf(stderr, "ERROR bad command\n");
                    } else {
                        // NOTE assume large enough buffer.
                        *(sm.end[0]) = 0;
                        *(sm.end[1]) = 0;
                        fprintf(stderr, "Add source [%s]\n", sm.beg[1]);
                        struct Handler *h2 = handler_list_find_by_input_addr_2(self->handler_list, sm.beg[1]);
                        if (h2 != NULL) {
                            fprintf(stderr, "ERROR source [%s] is already added\n", sm.beg[1]);
                        } else {
                            struct Handler *handler __attribute__((cleanup(cleanup_struct_Handler_ptr))) = NULL;
                            handler = malloc(sizeof(struct Handler));
                            NULLRET(handler);
                            handler->kind = SourceHandler;
                            ec = bsr_chnhandler_init(&handler->handler.src, poller, handler, sm.beg[1], self->stats);
                            NZRET(ec);
                            ec = handler_list_add(self->handler_list, handler);
                            NZRET(ec);
                            handler = NULL;
                        };
                    };
                    char *rep = "add-source: done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else if (n > 11 && memcmp("add-output,", buf, 11) == 0) {
                    struct SplitMap sm;
                    bsr_str_split((char *)buf, n, &sm);
                    if (sm.n != 3) {
                        fprintf(stderr, "ERROR wrong number of parameters\n");
                    } else {
                        buf[n] = 0;
                        *(sm.end[0]) = 0;
                        *(sm.end[1]) = 0;
                        *(sm.end[2]) = 0;
                        fprintf(stderr, "Add output  [%s]  [%s]  [%s]\n", sm.beg[0], sm.beg[1], sm.beg[2]);
                        struct bsr_chnhandler *h = handler_list_find_by_input_addr(self->handler_list, sm.beg[1]);
                        if (h == NULL) {
                            fprintf(stderr, "ERROR can not find handler for source %s\n", sm.beg[1]);
                        } else {
                            ec = bsr_chnhandler_add_out(h, sm.beg[2]);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR could not add output\n");
                            };
                        };
                    };
                    char *rep = "add-output: done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else if (n >= 13 && memcmp("remove-source", buf, 13) == 0) {
                    struct SplitMap sm;
                    bsr_str_split((char *)buf, n, &sm);
                    if (sm.n != 2) {
                        fprintf(stderr, "ERROR wrong number of parameters\n");
                    } else {
                        buf[n] = 0;
                        *(sm.end[0]) = 0;
                        *(sm.end[1]) = 0;
                        fprintf(stderr, "Remove source [%s] [%s]\n", sm.beg[0], sm.beg[1]);
                        struct Handler *h = handler_list_find_by_input_addr_2(self->handler_list, sm.beg[1]);
                        if (h == NULL) {
                            fprintf(stderr, "ERROR can not find handler for source %s\n", sm.beg[1]);
                        } else {
                            ec = handler_list_remove(self->handler_list, h);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR could not remove handler from handler-list\n");
                            } else {
                            };
                        };
                    };
                    char *rep = "remove-source: done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else if (n >= 13 && memcmp("remove-output", buf, 13) == 0) {
                    struct SplitMap sm;
                    bsr_str_split((char *)buf, n, &sm);
                    if (sm.n != 3) {
                        fprintf(stderr, "ERROR wrong number of parameters\n");
                    } else {
                        buf[n] = 0;
                        *(sm.end[0]) = 0;
                        *(sm.end[1]) = 0;
                        *(sm.end[2]) = 0;
                        fprintf(stderr, "Remove output [%s] [%s] [%s]\n", sm.beg[0], sm.beg[1], sm.beg[2]);
                        struct bsr_chnhandler *h = handler_list_find_by_input_addr(self->handler_list, sm.beg[1]);
                        if (h == NULL) {
                            fprintf(stderr, "ERROR can not find handler for source %s\n", sm.beg[1]);
                        } else {
                            ec = bsr_chnhandler_remove_out(h, sm.beg[2]);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR could not remove output\n");
                            };
                        };
                    };
                    char *rep = "remove-output: done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else {
                    char *rep = "Unknown command";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                };
                // TODO only try more if multipart. REQ must be matched by
                // REP.
                // do_recv = 0;
                // self->state = SEND;
                // ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLOUT);
                // TODO capture zmq error details. caller does not know whether
                // some returned error means that `errno` is set to some valid
                // value, and also not whether `errnor` might point to some
                // zmq-specific error-code.
                // if (ec == -1) {
                //    fprintf(stderr, "error: %d  %s\n", errno, zmq_strerror(errno));
                //};
                // NZRET(ec);
            };
        };
    } else if (self->state == SEND) {
        int n = snprintf((char *)buf, N, "reply");
        if (n <= 0) {
            return -1;
        };
        n = zmq_send(self->socket, buf, n, ZMQ_DONTWAIT);
        if (n == -1) {
            if (errno == EAGAIN) {
                // TODO should not happen with REQ/REP.
                fprintf(stderr, "ERROR EAGAIN on send\n");
                bsr_cmdchn_socket_reopen(self, poller);
                return 0;
            } else if (errno == EFSM) {
                fprintf(stderr, "ERROR command reply send: socket in wrong state\n");
                bsr_cmdchn_socket_reopen(self, poller);
                return 0;
            } else {
                fprintf(stderr, "ERROR on send %d\n", errno);
                bsr_cmdchn_socket_reopen(self, poller);
                return -1;
            };
        } else {
            // All good.
            self->state = RECV;
            ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLIN);
            NZRET(ec);
        };
    };
    return 0;
}
