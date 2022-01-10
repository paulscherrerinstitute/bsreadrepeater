/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

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
#define RCVHWM 128
#define RCVBUF (1024 * 128)
#define SNDHWM 128
#define SNDBUF (1024 * 128)

static int const NBUF = 1024 * 128;

int bsr_statistics_init(struct bsr_statistics *self) {
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    self->received = 0;
    self->sentok = 0;
    self->eagain = 0;
    self->eagain_multipart = 0;
    self->recv_buf_too_small = 0;
    self->poll_wait_ema = 0.0;
    self->poll_wait_emv = 0.0;
    return 0;
}

uint32_t bsr_statistics_ms_since_last_print(struct bsr_statistics *self) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    struct timespec t1 = self->last_print_ts;
    uint64_t dt =
        (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - (uint64_t)(t1.tv_sec * 1000 + t1.tv_nsec / 1000000);
    if (dt > UINT32_MAX) {
        dt = UINT32_MAX;
    };
    return dt;
}

ERRT bsr_statistics_print_now(struct bsr_statistics *self) {
    struct bsr_statistics *st = self;
    fprintf(stderr,
            "received %" PRIu64 "  sent %" PRIu64 "  busy %" PRIu64 "  busy-in-mp %" PRIu64
            "  recv_buf_too_small %" PRIu64 "\n",
            st->received, st->sentok, st->eagain, st->eagain_multipart, st->recv_buf_too_small);
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    return 0;
}

enum HandlerState {
    RECV = 1,
};

void cleanup_zmq_socket(void **k) {
    if (*k != NULL) {
        zmq_close(*k);
        *k = NULL;
    }
}

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self) {
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
            struct sockout *data = it->data;
            fprintf(stderr, "remove out sock it %p [%s]\n", (void *)it, data->addr);
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

ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, char *addr_inp,
                         struct bsr_statistics *stats) {
    int ec;
    self->poller = poller;
    self->user_data = user_data;
    strncpy(self->addr_inp, addr_inp, ADDR_CAP);
    self->addr_inp[ADDR_CAP - 1] = 0;
    self->state = RECV;
    self->received = 0;
    self->sentok = 0;
    self->eagain = 0;
    self->eagain_multipart = 0;
    self->recv_bytes = 0;
    self->sent_bytes = 0;
    self->stats = stats;
    self->socks_out = NULL;
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    self->sock_inp = zmq_socket(poller->ctx, ZMQ_PULL);
    ZMQ_NULLRET(self->sock_inp);
    ec = set_rcvhwm(self->sock_inp, RCVHWM);
    NZRET(ec);
    ec = set_rcvbuf(self->sock_inp, RCVBUF);
    NZRET(ec);
    ec = zmq_connect(self->sock_inp, self->addr_inp);
    ZMQ_NEGONE(ec);
    ec = zmq_poller_add(poller->poller, self->sock_inp, user_data, ZMQ_POLLIN);
    ZMQ_NEGONE(ec);
    return 0;
}

void cleanup_struct_sockout_ptr(struct sockout **k) {
    if (*k != NULL) {
        free(*k);
        *k = NULL;
    }
}

ERRT bsr_chnhandler_add_out(struct bsr_chnhandler *self, char *addr) {
    int ec;
    struct sockout *data __attribute__((cleanup(cleanup_struct_sockout_ptr))) = malloc(sizeof(struct sockout));
    NULLRET(data);
    strncpy(data->addr, addr, ADDR_CAP);
    data->addr[ADDR_CAP - 1] = 0;
    data->in_multipart = 0;
    data->sent_count = 0;
    data->sent_bytes = 0;
    data->eagain = 0;
    data->eagain_multipart = 0;
    void *sock __attribute__((cleanup(cleanup_zmq_socket))) = zmq_socket(self->poller->ctx, ZMQ_PUSH);
    ZMQ_NULLRET(sock);
    ec = set_sndhwm(sock, SNDHWM);
    NZRET(ec);
    ec = set_sndbuf(sock, SNDBUF);
    NZRET(ec);
    ec = zmq_bind(sock, data->addr);
    if (ec == -1) {
        fprintf(stderr, "ERROR can not bind to [%s]\n", data->addr);
        return -1;
    }
    ec = zmq_poller_add(self->poller->poller, sock, self->user_data, 0);
    ZMQ_NEGONE(ec);
    data->sock = sock;
    sock = NULL;
    self->socks_out = g_list_append(self->socks_out, data);
    data = NULL;
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
        free(data);
    };
    return 0;
}

ERRT bsr_chnhandler_remove_out(struct bsr_chnhandler *self, char *addr) {
    int ec;
    int i = 0;
    GList *it = self->socks_out;
    while (it != NULL) {
        struct sockout *data = it->data;
        if (strncmp(addr, data->addr, ADDR_CAP) == 0) {
            break;
        };
        it = it->next;
        i += 1;
    };
    ec = bsr_chnhandler_remove_nth_out(self, i);
    NZRET(ec);
    return 0;
}

void cleanup_zmq_msg(struct zmq_msg_t *k) {
    if (k != NULL) {
        zmq_msg_close(k);
    }
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
            zmq_msg_t msgin __attribute__((cleanup(cleanup_zmq_msg)));
            zmq_msg_init(&msgin);
            int n = zmq_msg_recv(&msgin, self->sock_inp, ZMQ_DONTWAIT);
            if (n == -1) {
                if (errno == EAGAIN) {
                    do_recv = 0;
                } else {
                    do_recv = 0;
                    fprintf(stderr, "error in recv %d  %s\n", errno, zmq_strerror(errno));
                    return -1;
                };
            } else {
                self->recv_bytes += n;
                char *buf = zmq_msg_data(&msgin);
                self->received += 1;
                self->stats->received += 1;
                if (n > NBUF) {
                    self->stats->recv_buf_too_small += 1;
                };
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
                    to_hex(sb, (uint8_t *)buf, nn);
                    fprintf(stderr, "Received: len %d  more %d  (%s) [%.*s]\n", n, more, sb, rawmax, buf);
                };
                int sndflags = ZMQ_DONTWAIT;
                if (more == 1) {
                    sndflags |= ZMQ_SNDMORE;
                };
                GList *it = self->socks_out;
                while (it != NULL) {
                    struct sockout *so = it->data;
                    zmq_msg_t msgout __attribute__((cleanup(cleanup_zmq_msg)));
                    ec = zmq_msg_init(&msgout);
                    ZMQ_NEGONE(ec);
                    ec = zmq_msg_copy(&msgout, &msgin);
                    ZMQ_NEGONE(ec);
                    int n2 = zmq_msg_send(&msgout, so->sock, sndflags);
                    if (n2 == -1) {
                        if (errno == EAGAIN) {
                            // Output can't keep up. Drop message.
                            if (so->in_multipart == 1) {
                                so->eagain_multipart += 1;
                                self->eagain_multipart += 1;
                                self->stats->eagain_multipart += 1;
                            } else {
                                so->eagain += 1;
                                self->eagain += 1;
                                self->stats->eagain += 1;
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
                        so->sent_count += 1;
                        so->sent_bytes += n2;
                        self->sentok += 1;
                        self->sent_bytes += n2;
                        self->stats->sentok += 1;
                        if (more == 1) {
                            so->in_multipart = 1;
                        } else {
                            so->in_multipart = 0;
                        };
                    };
                    it = it->next;
                };
            };
        };
    } else {
        fprintf(stderr, "chnhandler unknown state: %d\n", self->state);
        return -1;
    };
    if (bsr_statistics_ms_since_last_print(self->stats) > 4000) {
        bsr_statistics_print_now(self->stats);
    };
    return 0;
}
