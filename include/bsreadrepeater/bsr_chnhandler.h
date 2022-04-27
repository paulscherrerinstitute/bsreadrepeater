#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <bsr_ema.h>
#include <bsr_pod.h>
#include <bsr_poller.h>
#include <err.h>
#include <glib.h>
#include <zmq.h>

#define ADDR_CAP 80

struct sockout {
    void *sock;
    int sndhwm;
    int sndbuf;
    char addr[ADDR_CAP];
    int in_multipart;
    uint64_t sent_count;
    uint64_t sent_bytes;
    uint64_t eagain;
    uint64_t eagain_multipart;
};

struct bsread_main_header;

struct bsr_chnhandler {
    struct bsr_poller *poller;
    void *user_data;
    char addr_inp[ADDR_CAP];
    int inp_rcvhwm;
    int inp_rcvbuf;
    int state;
    int more_last;
    int mpc;
    uint64_t mpmsgc;
    uint32_t mpmsglen;
    void *sock_inp;
    // TODO better use a typed container:
    GList *socks_out;
    char *buf;
    int dh_compr;
    int printed_compr_unsup;
    struct timespec last_remember_channels;
    GArray *channels;
    struct channel_map *chnmap;
    struct bsread_main_header *bsread_main_header;
    struct bsread_data_header data_header;
    struct timespec ts_recv_last;
    uint64_t ts_main_header_last;
    uint64_t received;
    uint64_t mhparsed;
    uint64_t dhparsed;
    uint64_t dhdecompr;
    uint64_t sentok;
    uint64_t eagain;
    uint64_t eagain_multipart;
    uint64_t recv_bytes;
    uint64_t sent_bytes;
    uint64_t bsread_errors;
    uint64_t json_parse_errors;
    uint64_t data_header_lz4_count;
    uint64_t data_header_bslz4_count;
    uint64_t input_reopened;
    uint64_t throttling_enable_count;
    uint64_t timestamp_out_of_range_count;
    uint64_t unexpected_frames_count;
    // Shared, no need to clean up:
    struct bsr_statistics *stats;
    struct bsr_ema mpmsglen_ema;
    struct bsr_ema msg_dt_ema;
    struct bsr_ema_ext msg_emit_age;
    struct timespec ts_final_part_last;
    char input_disabled;
    char throttling;
    char block_current_mpm;
};

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, char const *addr_inp,
                         int hwm, int buf, struct bsr_statistics *stats);
ERRT bsr_chnhandler_connect(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_reopen_input(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_disable_input(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_enable_input(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_handle_event(struct bsr_chnhandler *self, short events, struct timespec tspoll,
                                 struct timespec ts_rt_poll_done);
ERRT bsr_chnhandler_add_out(struct bsr_chnhandler *self, char *addr, int sndhwm, int sndbuf);
ERRT bsr_chnhandler_remove_out(struct bsr_chnhandler *self, char *addr);
int bsr_chnhandler_outputs_count(struct bsr_chnhandler *self);
