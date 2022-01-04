#include <bsr_chnhandler.h>
#include <bsr_sockhelp.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PRINT_RECEIVED 0

static int const NBUF = 1024;

enum HandlerState {
    RECV = 1,
};

struct sockout {
    void *sock;
    int in_multipart;
};

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self) {
    fprintf(stderr, "INFO cleanup_bsr_chnhandler\n");
    int ec;
    if (self->sock_inp != NULL) {
        ec = zmq_poller_remove(self->poller->poller, self->sock_inp);
        if (ec == -1) {
            fprintf(stderr, "ERROR cleanup_bsr_chnhandler zmq_poller_remove inp %d\n", ec);
        }
        ec = zmq_close(self->sock_inp);
        if (ec == -1) {
            fprintf(stderr, "ERROR cleanup_bsr_chnhandler zmq_close inp %d\n", ec);
        }
        self->sock_inp = NULL;
        GList *it = self->socks_out;
        while (it != NULL) {
            fprintf(stderr, "remove out sock it %p\n", (void *)it);
            struct sockout *data = it->data;
            void *sock = data->sock;
            ec = zmq_poller_remove(self->poller->poller, sock);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup_bsr_chnhandler zmq_poller_remove out %d\n", ec);
            }
            ec = zmq_close(sock);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup_bsr_chnhandler zmq_close out %d %s\n", errno, zmq_strerror(errno));
            }
            free(data);
            it->data = NULL;
            it = it->next;
        }
        g_list_free(self->socks_out);
        self->socks_out = NULL;
    }
    return 0;
}

ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, int port) {
    int ec;
    self->poller = poller;
    self->user_data = user_data;
    self->state = RECV;
    self->sentok = 0;
    self->eagain = 0;
    self->socks_out = NULL;
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    self->buf = malloc(NBUF);
    if (self->buf == NULL) {
        return -1;
    }
    self->sock_inp = zmq_socket(poller->ctx, ZMQ_PULL);
    ZMQ_NULLRET(self->sock_inp);
    ec = set_rcvhwm(self->sock_inp, 10);
    NZRET(ec);
    ec = set_rcvbuf(self->sock_inp, 64);
    NZRET(ec);
    char buf[48];
    snprintf(buf, 48, "tcp://127.0.0.1:%d", port);
    ec = zmq_connect(self->sock_inp, buf);
    ZMQ_NEGONE(ec);
    ec = zmq_poller_add(poller->poller, self->sock_inp, user_data, ZMQ_POLLIN);
    ZMQ_NEGONE(ec);
    return 0;
}

void cleanup_zmq_socket(void **k) {
    if (*k != NULL) {
        zmq_close(*k);
        *k = NULL;
    }
}

ERRT bsr_chnhandler_add_out(struct bsr_chnhandler *self, int port) {
    int ec;
    void *sock __attribute__((cleanup(cleanup_zmq_socket))) = zmq_socket(self->poller->ctx, ZMQ_PUSH);
    ZMQ_NULLRET(sock);
    ec = set_sndhwm(sock, 100);
    NZRET(ec);
    ec = set_sndbuf(sock, 1024 * 2);
    NZRET(ec);
    char buf[48];
    snprintf(buf, 48, "tcp://127.0.0.1:%d", port);
    ec = zmq_bind(sock, buf);
    ZMQ_NEGONE(ec);
    ec = zmq_poller_add(self->poller->poller, sock, self->user_data, 0);
    ZMQ_NEGONE(ec);
    struct sockout *data = malloc(sizeof(struct sockout));
    data->sock = sock;
    sock = NULL;
    data->in_multipart = 0;
    self->socks_out = g_list_append(self->socks_out, data);
    return 0;
}

ERRT bsr_chnhandler_close_sock(struct bsr_chnhandler *self, void *sock) {
    int ec;
    ec = zmq_poller_remove(self->poller->poller, sock);
    ZMQ_NEGONE(ec);
    ec = zmq_close(sock);
    ZMQ_NEGONE(ec);
    return 0;
}

ERRT bsr_chnhandler_remove_nth_out(struct bsr_chnhandler *self, int ix) {
    int ec;
    GList *it = g_list_nth(self->socks_out, ix);
    if (it != NULL) {
        struct sockout *data = it->data;
        ec = bsr_chnhandler_close_sock(self, data->sock);
        NZRET(ec);
        self->socks_out = g_list_remove(self->socks_out, it->data);
    };
    return 0;
}

ERRT bsr_chnhandler_remove_out(struct bsr_chnhandler *self, int port) {
    port = port;
    int ec;
    ec = bsr_chnhandler_remove_nth_out(self, 0);
    NZRET(ec);
    return 0;
}

ERRT bsr_chnhandler_handle_event(struct bsr_chnhandler *self, struct bsr_poller *poller) {
    poller = poller;
    int ec;
    int sockflags;
    {
        size_t vl = sizeof(sockflags);
        ec = zmq_getsockopt(self->sock_inp, ZMQ_EVENTS, &sockflags, &vl);
    }
    if (self->state == RECV) {
        if ((sockflags & ZMQ_POLLIN) == 0) {
            fprintf(stderr, "RECV but not POLLIN\n");
            return -1;
        };
        int do_recv = 1;
        while (do_recv) {
            int n = zmq_recv(self->sock_inp, self->buf, NBUF, ZMQ_DONTWAIT);
            if (n == -1) {
                if (errno == EAGAIN) {
                    do_recv = 0;
                } else {
                    do_recv = 0;
                    fprintf(stderr, "error in recv %d  %s\n", errno, zmq_strerror(errno));
                    return -1;
                }
            } else {
                int more;
                {
                    size_t opt_val_len = sizeof(more);
                    ec = zmq_getsockopt(self->sock_inp, ZMQ_RCVMORE, &more, &opt_val_len);
                    ZMQ_NEGONE(ec);
                }
                if (PRINT_RECEIVED) {
                    int const N2 = 6;
                    char sb[2 * N2 + 4];
                    int nn = N2;
                    if (n < nn) {
                        nn = n;
                    };
                    int rawmax = 7;
                    if (n < rawmax) {
                        rawmax = n;
                    };
                    to_hex(sb, (uint8_t *)self->buf, nn);
                    fprintf(stderr, "Received: len %d  more %d  (%s) [%.*s]\n", n, more, sb, rawmax, self->buf);
                }
                int sndflags = ZMQ_DONTWAIT;
                if (more == 1) {
                    sndflags |= ZMQ_SNDMORE;
                }
                GList *it = self->socks_out;
                while (it != NULL) {
                    struct sockout *data = it->data;
                    void *so = data->sock;
                    int n2 = zmq_send(so, self->buf, n, sndflags);
                    if (n2 == -1) {
                        if (errno == EAGAIN) {
                            if (data->in_multipart == 1) {
                                self->eagain_multipart += 1;
                            } else {
                                // Output can't keep up. Drop message.
                                self->eagain += 1;
                            }
                        } else {
                            fprintf(stderr, "chnhandler send error: %d %s\n", errno, zmq_strerror(errno));
                            return -1;
                        };
                    } else if (n2 != n) {
                        fprintf(stderr, "chnhandler zmq_send byte mismatch %d vs %d\n", n2, n);
                        return -1;
                    } else {
                        // Message is accepted by zmq.
                        self->sentok += 1;
                        if (more == 1) {
                            data->in_multipart = 1;
                        } else {
                            data->in_multipart = 0;
                        };
                    };
                    it = it->next;
                }
            }
        };
    } else {
        fprintf(stderr, "chnhandler unknown state: %d\n", self->state);
        return -1;
    };
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t dt =
        (ts.tv_sec - self->last_print_ts.tv_sec) * 1000 + (ts.tv_nsec - self->last_print_ts.tv_nsec) / 1000000;
    if (dt > 1500) {
        fprintf(stderr, "sent: %" PRIu64 "  busy: %" PRIu64 "  busy-in-mp: %" PRIu64 "\n", self->sentok, self->eagain,
                self->eagain_multipart);
        self->last_print_ts = ts;
    };
    return 0;
}
