/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bitshuffle.h>
#include <bsr_chnhandler.h>
#include <bsr_json.h>
#include <bsr_sockhelp.h>
#include <bsr_stats.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
#include <inttypes.h>
#include <lz4.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PRINT_RECEIVED 0
#define RCVHWM 128
#define RCVBUF (1024 * 128)
#define SNDHWM 128
#define SNDBUF (1024 * 128)

static int const NBUF = 1024 * 128;
static int const DECOMP_BUF_MAX = 1024 * 128;

enum HandlerState {
    RECV = 1,
};

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self) {
    int ec;
    if (self->buf != NULL) {
        free(self->buf);
        self->buf = NULL;
    }
    if (self->channels != NULL) {
        g_array_free(self->channels, TRUE);
        self->channels = NULL;
    }
    if (self->chnmap != NULL) {
        channel_map_release(self->chnmap);
        self->chnmap = NULL;
    }
    if (self->bsread_main_header != NULL) {
        free(self->bsread_main_header);
        self->bsread_main_header = NULL;
    }
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

void clear_channel_element(void *k) {
    char **ptr = (char **)k;
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

void cleanup_struct_sockout_ptr(struct sockout **k) {
    if (*k != NULL) {
        free(*k);
        *k = NULL;
    }
}

ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, char *addr_inp,
                         struct bsr_statistics *stats) {
    int ec;
    self->poller = poller;
    self->user_data = user_data;
    strncpy(self->addr_inp, addr_inp, ADDR_CAP);
    self->addr_inp[ADDR_CAP - 1] = 0;
    self->state = RECV;
    self->more_last = 0;
    self->mpc = 0;
    self->mpmsgc = 0;
    self->printed_compr_unsup = 0;
    self->received = 0;
    self->sentok = 0;
    self->eagain = 0;
    self->eagain_multipart = 0;
    self->recv_bytes = 0;
    self->sent_bytes = 0;
    self->bsread_errors = 0;
    self->mhparsed = 0;
    self->dhparsed = 0;
    self->dhdecompr = 0;
    self->json_parse_errors = 0;
    self->stats = stats;
    self->socks_out = NULL;
    self->buf = malloc(DECOMP_BUF_MAX);
    NULLRET(self->buf);
    clock_gettime(CLOCK_MONOTONIC, &self->last_remember_channels);
    self->channels = g_array_new(FALSE, FALSE, sizeof(void *));
    NULLRET(self->channels);
    self->chnmap = channel_map_new();
    NULLRET(self->chnmap);
    self->bsread_main_header = malloc(sizeof(struct bsread_main_header));
    NULLRET(self->bsread_main_header);
    self->bsread_main_header->compr = 0;
    self->bsread_main_header->pulse = 0;
    self->bsread_main_header->timestamp = 0;
    g_array_set_clear_func(self->channels, clear_channel_element);
    self->sock_inp = zmq_socket(poller->ctx, ZMQ_PULL);
    ZMQ_NULLRET(self->sock_inp);
    ec = set_basic_sock_opts(self->sock_inp);
    NZRET(ec);
    ec = set_rcvhwm(self->sock_inp, RCVHWM);
    NZRET(ec);
    ec = set_rcvbuf(self->sock_inp, RCVBUF);
    NZRET(ec);
    ec = zmq_connect(self->sock_inp, self->addr_inp);
    ZMQ_NEGONERET(ec);
    ec = zmq_poller_add(poller->poller, self->sock_inp, user_data, ZMQ_POLLIN);
    ZMQ_NEGONERET(ec);
    return 0;
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
    ZMQ_NEGONERET(ec);
    data->sock = sock;
    sock = NULL;
    self->socks_out = g_list_append(self->socks_out, data);
    data = NULL;
    return 0;
}

ERRT bsr_chnhandler_close_sock(struct bsr_chnhandler *self, void *sock) {
    int ec;
    ec = zmq_poller_remove(self->poller->poller, sock);
    ZMQ_NEGONERET(ec);
    ec = zmq_close(sock);
    ZMQ_NEGONERET(ec);
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
    }
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
    }
    ec = bsr_chnhandler_remove_nth_out(self, i);
    NZRET(ec);
    return 0;
}

void cleanup_zmq_msg(struct zmq_msg_t *k) {
    if (k != NULL) {
        zmq_msg_close(k);
    }
}

void cleanup_gstring(GString **k) {
    if (*k != NULL) {
        g_string_free(*k, TRUE);
        *k = NULL;
    }
}

ERRT bsr_chnhandler_handle_event(struct bsr_chnhandler *self, struct bsr_poller *poller, struct timespec tspoll) {
    poller = poller;
    int ec;
    uint64_t now = tspoll.tv_sec * 1000000 + tspoll.tv_nsec / 1000;
    int sockflags;
    {
        size_t vl = sizeof(sockflags);
        ec = zmq_getsockopt(self->sock_inp, ZMQ_EVENTS, &sockflags, &vl);
    }
    if (self->state == RECV) {
        if ((sockflags & ZMQ_POLLIN) == 0) {
            fprintf(stderr, "RECV but not POLLIN\n");
            return -1;
        }
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
                }
            } else {
                self->recv_bytes += n;
                self->stats->recv_bytes += n;
                char *buf = zmq_msg_data(&msgin);
                self->received += 1;
                self->stats->received += 1;
                if (n > NBUF) {
                    self->stats->recv_buf_too_small += 1;
                }
                int more = zmq_msg_more(&msgin);
                int const NHEXBUF = 16;
                char hexbuf[2 * NHEXBUF + 4];
                if (PRINT_RECEIVED) {
                    int nn = NHEXBUF;
                    if (n < nn) {
                        nn = n;
                    };
                    int rawmax = 7;
                    if (n < rawmax) {
                        rawmax = n;
                    };
                    to_hex(hexbuf, (uint8_t *)buf, nn);
                    fprintf(stderr, "Received: len %d  more %d  (%s) [%.*s]\n", n, more, hexbuf, rawmax, buf);
                }
                if (self->more_last == 1) {
                    self->mpc += 1;
                } else {
                    self->mpc = 0;
                }
                int sndflags = ZMQ_DONTWAIT;
                if (more == 1) {
                    sndflags |= ZMQ_SNDMORE;
                } else {
                }
                if (self->mpc == 0) {
                    self->dh_compr = 0;
                    GString *log __attribute__((cleanup(cleanup_gstring))) = g_string_new("");
                    struct bsread_main_header *header = self->bsread_main_header;
                    if (json_parse_main_header(buf, n, header, &log) != 0) {
                        self->json_parse_errors += 1;
                        if (self->json_parse_errors < 10) {
                            to_hex(hexbuf, (uint8_t *)buf, NHEXBUF);
                            fprintf(stderr, "can not parse  %s  (%d, %d)  %d  %.*s  [%s]\n", self->addr_inp,
                                    self->mpmsgc, self->mpc, n, 32, buf, hexbuf);
                        }
                    } else {
                        self->mhparsed += 1;
                        self->dh_compr = header->compr;
                        if (FALSE && (self->mpmsgc % 300) == 0) {
                            fprintf(stderr, "GOOD PARSE     %s  (%d, %d)\ncompr: %d\n%s\n-----\n", self->addr_inp,
                                    self->mpmsgc, self->mpc, header->compr, log->str);
                        }
                    }
                }
                if (self->mpc == 1) {
                    char const *mb = (char *)zmq_msg_data(&msgin);
                    char const *jsstr = mb;
                    int jslen = n;
                    GString *log __attribute__((cleanup(cleanup_gstring))) = g_string_new("");
                    if (self->dh_compr == 1) {
                        if (self->printed_compr_unsup < 10) {
                            fprintf(stderr, "ERROR bitshuffle not yet supported for data header compression\n");
                            self->printed_compr_unsup += 1;
                        }
                    }
                    if (self->dh_compr == 2) {
                        int b;
                        char *p = (char *)&b;
                        p[0] = mb[3];
                        p[1] = mb[2];
                        p[2] = mb[1];
                        p[3] = mb[0];
                        ec = LZ4_decompress_safe(mb + 4, self->buf, n - 4, DECOMP_BUF_MAX);
                        if (ec < 0) {
                            if (self->printed_compr_unsup < 10) {
                                fprintf(stderr, "lz4 decompress error %d  len %d\n", ec, b);
                                self->printed_compr_unsup += 1;
                            }
                        } else {
                            jsstr = self->buf;
                            jslen = b;
                            self->dhdecompr += 1;
                        }
                    }
                    struct channel_map *chnmap = NULL;
                    if (TRUE || (tspoll.tv_sec >= self->last_remember_channels.tv_sec + 2)) {
                        self->last_remember_channels = tspoll;
                        chnmap = self->chnmap;
                    }
                    struct bsread_data_header header = {0};
                    if (json_parse_data_header(jsstr, jslen, &header, now, chnmap, self->bsread_main_header, &log) !=
                        0) {
                        self->json_parse_errors += 1;
                        if (self->json_parse_errors < 10) {
                            to_hex(hexbuf, (uint8_t *)buf, NHEXBUF);
                            fprintf(stderr, "can not parse  %s  (%d, %d)  %d  %.*s  [%s]\n", self->addr_inp,
                                    self->mpmsgc, self->mpc, n, 32, mb, hexbuf);
                        }
                    } else {
                        self->dhparsed += 1;
                        if (FALSE && (self->mpmsgc % 300) == 0) {
                            fprintf(stderr, "GOOD PARSE     %s  (%d, %d)\n%s\n-----\n", self->addr_inp, self->mpmsgc,
                                    self->mpc, log->str);
                        }
                    }
                }
                if (self->mpmsgc > 0) {
                    GList *it = self->socks_out;
                    while (it != NULL) {
                        struct sockout *so = it->data;
                        zmq_msg_t msgout __attribute__((cleanup(cleanup_zmq_msg)));
                        ec = zmq_msg_init(&msgout);
                        ZMQ_NEGONERET(ec);
                        ec = zmq_msg_copy(&msgout, &msgin);
                        ZMQ_NEGONERET(ec);
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
                            self->stats->sent_bytes += n2;
                            if (more == 1) {
                                so->in_multipart = 1;
                            } else {
                                so->in_multipart = 0;
                            }
                        };
                        it = it->next;
                    }
                }
                self->more_last = more;
                if (more == 0) {
                    self->mpmsgc += 1;
                }
            }
        }
    } else {
        fprintf(stderr, "chnhandler unknown state: %d\n", self->state);
        return -1;
    }
    if (bsr_statistics_ms_since_last_print(self->stats) > 4000) {
        bsr_statistics_print_now(self->stats);
    }
    return 0;
}
