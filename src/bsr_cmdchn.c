#include <bsr_cmdchn.h>
#include <bsr_dummy_source.h>
#include <bsr_json.h>
#include <bsr_json_commands.h>
#include <bsr_stats.h>
#include <bsr_str.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char const *const RECONNECT_SOURCE = "reconnect-source";

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
    ZMQ_NEGONERET(ec);
    ec = zmq_poller_add(poller->poller, self->socket, self->user_data, ZMQ_POLLIN);
    ZMQ_NEGONERET(ec);
    return 0;
}

ERRT bsr_cmdchn_socket_reopen(struct bsr_cmdchn *self, struct bsr_poller *poller) {
    int ec = bsr_cmdchn_socket_reopen_inner(self, poller);
    if (ec != 0) {
        fprintf(stderr, "ERROR can not reopen\n");
    }
    return 0;
}

static void cleanup_struct_Handler_ptr(struct Handler **k) {
    if (*k != NULL) {
        cleanup_struct_Handler(*k);
        free(*k);
        *k = NULL;
    }
}

void cleanup_struct_Handler(struct Handler *self) {
    int ec;
    if (self != NULL && self->kind != 0) {
        switch (self->kind) {
        case CommandHandler: {
            cleanup_bsr_cmdchn(&self->handler.cmdh.cmdchn);
            break;
        }
        case SourceHandler: {
            ec = cleanup_bsr_chnhandler(&self->handler.src);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup SourceHandler\n");
            }
            break;
        }
        case StartupHandler: {
            ec = cleanup_struct_bsr_startupcmd(&self->handler.start);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup StartupHandler\n");
            }
            break;
        }
        case MemReqHandler: {
            ec = cleanup_struct_bsr_memreq(&self->handler.memreq);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup MemReqHandler\n");
            }
            break;
        }
        case DummySourceHandler: {
            ec = dummy_source_cleanup(&self->handler.dummy_source);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup DummySourceHandler\n");
            }
            break;
        }
        default: {
            fprintf(stderr, "ERROR cleanup_struct_Handler unknown kind %d\n", self->kind);
        }
        }
        self->kind = 0;
    } else {
        fprintf(stderr, "WARN cleanup_struct_Handler nothing to do??\n");
    }
}

void cleanup_free_struct_Handler(struct Handler **k) {
    if (*k != NULL) {
        free(*k);
        *k = NULL;
    }
}

ERRT cleanup_struct_HandlerList(struct HandlerList *self) {
    GList *it = self->list;
    while (it != NULL) {
        cleanup_struct_Handler(it->data);
        // TODO this assumes that items in this list are always on the heap.
        free(it->data);
        it->data = NULL;
        it = it->next;
    }
    g_list_free(self->list);
    self->list = NULL;
    return -1;
}

ERRT handler_list_init(struct HandlerList *self) {
    self->list = NULL;
    return 0;
}

ERRT handler_list_add(struct HandlerList *self, struct Handler *handler) {
    // TODO currently we assume that `handler` is on the heap and we assume ownership.
    self->list = g_list_append(self->list, handler);
    return 0;
}

ERRT handler_list_remove(struct HandlerList *self, struct Handler *handler) {
    cleanup_struct_Handler(handler);
    self->list = g_list_remove(self->list, handler);
    // TODO check that HandlerList owns the contained struct Handler.
    free(handler);
    return 0;
}

struct Handler *handler_list_get_data(GList *p) { return (struct Handler *)p->data; }

struct bsr_chnhandler *handler_list_find_by_input_addr(struct HandlerList *self, char const *addr) {
    GList *it = self->list;
    while (it != NULL) {
        struct Handler *handler = it->data;
        if (handler->kind == SourceHandler) {
            if (strncmp(handler->handler.src.addr_inp, addr, ADDR_CAP) == 0) {
                return &handler->handler.src;
            };
        };
        it = it->next;
    }
    return NULL;
}

struct Handler *handler_list_find_by_input_addr_2(struct HandlerList *self, char const *addr) {
    GList *it = self->list;
    while (it != NULL) {
        struct Handler *handler = it->data;
        if (handler->kind == SourceHandler) {
            if (strncmp(handler->handler.src.addr_inp, addr, ADDR_CAP) == 0) {
                return handler;
            };
        };
        it = it->next;
    }
    return NULL;
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
                }
            } else {
                buf[n] = 0;
                fprintf(stderr, "Received command: %d [%.*s]\n", n, n, buf);
                if (n > 1 && buf[0] == '{') {
                    struct bsr_json_command cmd;
                    ec = parse_json_command((char const *)buf, n, &cmd);
                    if (ec != 0) {
                        fprintf(stderr, "JSON command parse error\n");
                        char const *rep = "JSON parse error";
                        zmq_send(self->socket, rep, strlen(rep), 0);
                    } else {
                        fprintf(stderr, "OK JSON command\n");
                        if (cmd.kind == ADD_SOURCE) {
                            struct command_add_source cmd2 = cmd.inner.add_source;
                            char const *src = cmd2.source;
                            fprintf(stderr, "Add source [%s]\n", src);
                            struct Handler *h2 = handler_list_find_by_input_addr_2(self->handler_list, src);
                            if (h2 != NULL) {
                                fprintf(stderr, "ERROR source [%s] is already added\n", src);
                            } else {
                                struct Handler *handler __attribute__((cleanup(cleanup_struct_Handler_ptr))) = NULL;
                                handler = malloc(sizeof(struct Handler));
                                NULLRET(handler);
                                handler->kind = SourceHandler;
                                ec = bsr_chnhandler_init(&handler->handler.src, poller, handler, src, cmd2.rcvhwm,
                                                         cmd2.rcvbuf, self->stats);
                                if (ec != 0) {
                                    fprintf(stderr, "ERROR can not initialize handler for [%s]\n", src);
                                } else {
                                    ec = bsr_chnhandler_connect(&handler->handler.src);
                                    if (ec != 0) {
                                        fprintf(stderr, "ERROR can not connect handler for [%s]\n", src);
                                    } else {
                                        ec = handler_list_add(self->handler_list, handler);
                                        NZRET(ec);
                                        handler = NULL;
                                    }
                                }
                            }
                        } else if (cmd.kind == ADD_OUTPUT) {
                            struct command_add_output cmd2 = cmd.inner.add_output;
                            fprintf(stderr, "add-output  [%s]  [%s]  %d  %d\n", cmd2.source, cmd2.output, cmd2.sndhwm,
                                    cmd2.sndbuf);
                            struct bsr_chnhandler *h = handler_list_find_by_input_addr(self->handler_list, cmd2.source);
                            if (h == NULL) {
                                fprintf(stderr, "ERROR can not find handler for source %s\n", cmd2.source);
                            } else {
                                ec = bsr_chnhandler_add_out(h, cmd2.output, cmd2.sndhwm, cmd2.sndbuf);
                                if (ec != 0) {
                                    fprintf(stderr, "ERROR could not add output\n");
                                }
                            }
                        } else if (cmd.kind == DUMMY_SOURCE_START) {
                            struct Handler *handler __attribute__((cleanup(cleanup_struct_Handler_ptr))) = NULL;
                            handler = malloc(sizeof(struct Handler));
                            memset(handler, 0, sizeof(*handler));
                            NULLRET(handler);
                            handler->kind = DummySourceHandler;
                            struct bsr_dummy_source *dummy = &handler->handler.dummy_source;
                            fprintf(stderr, "DO INIT\n");
                            ec = dummy_source_init(dummy, poller);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR can not initialize DummySourceHandler\n");
                            } else {
                                ec = handler_list_add(self->handler_list, handler);
                                NZRET(ec);
                                handler = NULL;
                            }
                        }
                        char const *rep = "received json command";
                        zmq_send(self->socket, rep, strlen(rep), 0);
                    }
                } else if (n == 4 && memcmp("exit", buf, n) == 0) {
                    cmd->ty = CmdExit;
                    zmq_send(self->socket, "exit", 4, 0);
                } else if (n == 5 && memcmp("stats", buf, 5) == 0) {
                    GString *str = g_string_new("");
                    char s[256];
                    snprintf(s, sizeof(s), "received %" PRIu64 "\n", self->stats->received);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "sent %" PRIu64 "\n", self->stats->sentok);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "recv bytes %" PRIu64 "\n", self->stats->recv_bytes);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "sent bytes %" PRIu64 "\n", self->stats->sent_bytes);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "small buf %" PRIu64 "\n", self->stats->recv_buf_too_small);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "busy %" PRIu64 "\n", self->stats->eagain);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "busy mp %" PRIu64 "\n", self->stats->eagain_multipart);
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "poll avg %7.0f var %7.0f\n", self->stats->poll_wait_ema,
                             sqrtf(self->stats->poll_wait_emv));
                    str = g_string_append(str, s);
                    snprintf(s, sizeof(s), "process avg %7.0f var %7.0f\n", self->stats->process_ema,
                             sqrtf(self->stats->process_emv));
                    str = g_string_append(str, s);
                    str = g_string_append(str, "idle check last ");
                    struct tm tms;
                    gmtime_r(&self->stats->datetime_idle_check_last.tv_sec, &tms);
                    strftime(s, sizeof(s), "%Y-%m-%dT%H:%M:%SZ", &tms);
                    str = g_string_append(str, s);
                    str = g_string_append(str, "\n");
                    snprintf(s, sizeof(s), "input reopened %" PRIu64 "\n", self->stats->input_reopened);
                    str = g_string_append(str, s);
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
                            // str = g_string_append(str, "+++++  channel map begin\n");
                            // channel_map_str(h->chnmap, &str);
                            // str = g_string_append(str, "-----  channel map end\n");
                            GList *it2 = h->socks_out;
                            while (it2 != NULL) {
                                struct sockout *so = it2->data;
                                snprintf(s, sizeof(s), "  Out: %s\n", so->addr);
                                str = g_string_append(str, s);
                                snprintf(s, sizeof(s), "    sent count %" PRIu64 "\n", so->sent_count);
                                str = g_string_append(str, s);
                                snprintf(s, sizeof(s), "    sent bytes %" PRIu64 "\n", so->sent_bytes);
                                str = g_string_append(str, s);
                                snprintf(s, sizeof(s), "    busy %" PRIu64 "\n", so->eagain);
                                str = g_string_append(str, s);
                                snprintf(s, sizeof(s), "    busy mp %" PRIu64 "\n", so->eagain_multipart);
                                str = g_string_append(str, s);
                                it2 = it2->next;
                            }
                            struct stats_source_pub st = {0};
                            st.received = h->received;
                            st.mhparsed = h->mhparsed;
                            st.dhparsed = h->dhparsed;
                            st.json_parse_errors = h->json_parse_errors;
                            st.dhdecompr = h->dhdecompr;
                            st.data_header_lz4_count = h->data_header_lz4_count;
                            st.data_header_bslz4_count = h->data_header_bslz4_count;
                            st.sentok = h->sentok;
                            st.recv_bytes = h->recv_bytes;
                            st.sent_bytes = h->sent_bytes;
                            st.bsread_errors = h->bsread_errors;
                            {
                                st.msg_dt_ema = h->msg_dt_ema.ema;
                                st.msg_dt_emv = h->msg_dt_ema.emv;
                            }
                            {
                                // TODO make a method for this kind of access: get_and_reset or something.
                                st.msg_emit_age_ema = h->msg_emit_age.ema;
                                st.msg_emit_age_emv = h->msg_emit_age.emv;
                                st.msg_emit_age_min = h->msg_emit_age.min;
                                st.msg_emit_age_max = h->msg_emit_age.max;
                                h->msg_emit_age.min = INFINITY;
                                h->msg_emit_age.max = -INFINITY;
                            }
                            str = g_string_truncate(str, 0);
                            ec = stats_source_to_json(&st, &str);
                            if (ec == -1) {
                                fprintf(stderr, "stats json encode issue\n");
                            }
                        }
                    }
                    zmq_send(self->socket, str->str, str->len, 0);
                    g_string_free(str, TRUE);
                } else if (n >= 12 && memcmp("list-sources", buf, 12) == 0) {
                    GList *it = self->handler_list->list;
                    GString *str = g_string_new("");
                    while (it != NULL) {
                        struct Handler *handler = it->data;
                        if (handler->kind == SourceHandler) {
                            struct bsr_chnhandler *ch = &handler->handler.src;
                            str = g_string_append(str, ch->addr_inp);
                            str = g_string_append(str, "\n");
                            GList *it2 = ch->socks_out;
                            while (it2 != NULL) {
                                struct sockout *so = it2->data;
                                str = g_string_append(str, " Out: ");
                                str = g_string_append(str, so->addr);
                                str = g_string_append(str, "\n");
                                it2 = it2->next;
                            }
                        }
                        it = it->next;
                    }
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
                            ec = bsr_chnhandler_init(&handler->handler.src, poller, handler, sm.beg[1], 200,
                                                     1024 * 1024 * 20, self->stats);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR can not initialize handler for [%s]\n", sm.beg[1]);
                            } else {
                                ec = bsr_chnhandler_connect(&handler->handler.src);
                                if (ec != 0) {
                                    fprintf(stderr, "ERROR can not connect handler for [%s]\n", sm.beg[1]);
                                } else {
                                    ec = handler_list_add(self->handler_list, handler);
                                    NZRET(ec);
                                    handler = NULL;
                                }
                            }
                        }
                    }
                    char *rep = "add-source: done (rev2)";
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
                            ec = bsr_chnhandler_add_out(h, sm.beg[2], 140, 1024 * 128);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR could not add output\n");
                            }
                        }
                    }
                    char *rep = "add-output: done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else if (n >= 13 && memcmp("remove-source", buf, 13) == 0) {
                    // TODO factor in function call to catch errors and report to client:
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
                            }
                        }
                    }
                    char *rep = "done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else if ((size_t)n >= strlen(RECONNECT_SOURCE) &&
                           memcmp(RECONNECT_SOURCE, buf, strlen(RECONNECT_SOURCE)) == 0) {
                    // TODO factor in function call to catch errors and report to client:
                    struct SplitMap sm;
                    bsr_str_split((char *)buf, n, &sm);
                    if (sm.n != 2) {
                        fprintf(stderr, "ERROR wrong number of parameters\n");
                    } else {
                        buf[n] = 0;
                        *(sm.end[0]) = 0;
                        *(sm.end[1]) = 0;
                        fprintf(stderr, "Reconnect source [%s] [%s]\n", sm.beg[0], sm.beg[1]);
                        struct Handler *h = handler_list_find_by_input_addr_2(self->handler_list, sm.beg[1]);
                        if (h == NULL) {
                            fprintf(stderr, "ERROR can not find handler for source %s\n", sm.beg[1]);
                        } else {
                            if (h->kind == SourceHandler) {
                                struct bsr_chnhandler *h2 = &h->handler.src;
                                ec = bsr_chnhandler_reopen_input(h2);
                                if (ec != 0) {
                                    // TODO return error message
                                } else {
                                    // TODO return all-good message
                                }
                            } else {
                                // TODO return error message
                            }
                        }
                    }
                    char *rep = "done";
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
                            }
                        }
                    }
                    char *rep = "remove-output: done";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                } else {
                    char *rep = "Unknown command";
                    zmq_send(self->socket, rep, strlen(rep), 0);
                }
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
                //}
                // NZRET(ec);
            }
        }
    } else if (self->state == SEND) {
        int n = snprintf((char *)buf, N, "reply");
        if (n <= 0) {
            return -1;
        }
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
            }
        } else {
            // All good.
            self->state = RECV;
            ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLIN);
            NZRET(ec);
        }
    }
    return 0;
}

ERRT handlers_handle_msg(struct Handler *self, short events, struct bsr_poller *poller, struct ReceivedCommand *cmd,
                         struct timespec tspoll, struct timespec ts_rt_poll_done) {
    int ec;
    if (0) {
        fprintf(stderr, "handlers_handle_msg  kind %d  self %p\n", self->kind, (void *)self);
    }
    switch (self->kind) {
    case CommandHandler: {
        struct CommandHandler *h = &self->handler.cmdh;
        ec = bsr_cmdchn_handle_event(&h->cmdchn, poller, cmd);
        NZRET(ec);
        break;
    }
    case SourceHandler: {
        ec = bsr_chnhandler_handle_event(&self->handler.src, events, tspoll, ts_rt_poll_done);
        NZRET(ec);
        break;
    }
    case StartupHandler: {
        ec = bsr_startupcmd_handle_event(&self->handler.start);
        NZRET(ec);
        break;
    }
    case MemReqHandler: {
        ec = bsr_memreq_handle_event(&self->handler.memreq, events);
        NZRET(ec);
        break;
    }
    default: {
        fprintf(stderr, "ERROR handlers_handle_msg unhandled kind %d\n", self->kind);
    }
    }
    return 0;
}
