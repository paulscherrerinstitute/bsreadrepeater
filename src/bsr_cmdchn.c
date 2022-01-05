/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_cmdchn.h>
#include <bsr_str.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
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

int bsr_cmdchn_init(struct bsr_cmdchn *self, struct bsr_poller *poller, void *user_data, int port,
                    struct HandlerList *handler_list) {
    int ec;
    self->state = RECV;
    self->handler_list = handler_list;
    // self->socket = zmq_socket(poller->ctx, ZMQ_STREAM);
    self->socket = zmq_socket(poller->ctx, ZMQ_REP);
    if (self->socket == NULL) {
        fprintf(stderr, "can not create socket: %s\n", zmq_strerror(errno));
        return -1;
    }
    char buf[32];
    snprintf(buf, 32, "tcp://127.0.0.1:%d", port);
    ec = zmq_bind(self->socket, buf);
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

void cleanup_struct_Handler_ptr(struct Handler **k) {
    if (*k != NULL) {
        // TODO cleanup the interior as well if needed.
        free(*k);
        *k = NULL;
    }
}

int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, struct bsr_poller *poller, struct ReceivedCommand *cmd) {
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
                    fprintf(stderr, "no more\n");
                    break;
                } else {
                    fprintf(stderr, "error in recv %d  %s\n", errno, zmq_strerror(errno));
                    return -1;
                }
            } else {
                // char sb[256];
                // to_hex(sb, buf, n);
                fprintf(stderr, "Received command: %d [%.*s]\n", n, n, buf);
                if (n == 4 && memcmp("exit", buf, n) == 0) {
                    cmd->ty = CmdExit;
                    zmq_send(self->socket, "exit", 4, 0);
                } else if (n > 11 && memcmp("add-source,", buf, 11) == 0) {
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
                            ec = bsr_chnhandler_init(&handler->handler.src, poller, handler, sm.beg[1]);
                            NZRET(ec);
                            ec = handler_list_add(self->handler_list, handler);
                            NZRET(ec);
                            handler = NULL;
                        };
                    };
                } else if (n > 11 && memcmp("add-output,", buf, 11) == 0) {
                    int i2 = 0;
                    char *splits[4];
                    char *p1 = (char *)buf;
                    while (p1 < (char *)buf + n - 1) {
                        if (*p1 == ',') {
                            splits[i2] = p1 + 1;
                            i2 += 1;
                        };
                        p1 += 1;
                        if (i2 >= 4) {
                            break;
                        };
                    };
                    if (i2 != 2) {
                        fprintf(stderr, "ERROR wrong number of parameters\n");
                    } else {
                        // NOTE assume large enough buffer.
                        buf[n] = 0;
                        *(splits[0] - 1) = 0;
                        *(splits[1] - 1) = 0;
                        fprintf(stderr, "Add output  [%s]  [%s]  [%s]\n", buf, splits[0], splits[1]);
                        struct bsr_chnhandler *h = handler_list_find_by_input_addr(self->handler_list, splits[0]);
                        if (h == NULL) {
                            fprintf(stderr, "ERROR can not find handler for source %s\n", splits[0]);
                        } else {
                            ec = bsr_chnhandler_add_out(h, splits[1]);
                            if (ec != 0) {
                                fprintf(stderr, "ERROR could not add output\n");
                            };
                        };
                    };
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
                            };
                        };
                    };
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
                };
                // TODO only try more if multipart. REQ must be matched by
                // REP.
                do_recv = 0;
                self->state = SEND;
                ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLOUT);
                // TODO capture zmq error details. caller does not know whether
                // some returned error means that `errno` is set to some valid
                // value, and also not whether `errnor` might point to some
                // zmq-specific error-code.
                if (ec == -1) {
                    fprintf(stderr, "error: %d  %s\n", errno, zmq_strerror(errno));
                };
                NZRET(ec);
            }
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
                fprintf(stderr, "EAGAIN on send\n");
                return 0;
            } else {
                fprintf(stderr, "ERROR on send %d\n", errno);
                return -1;
            };
        } else {
            self->state = RECV;
            ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLIN);
            NZRET(ec);
        };
    };
    return 0;
}
