/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <glib.h>
#include <zmq.h>

#define ADDR_CAP 80

struct bsr_statistics {
    uint64_t received;
    uint64_t sentok;
    uint64_t eagain;
    uint64_t eagain_multipart;
    uint64_t recv_buf_too_small;
    struct timespec last_print_ts;
};

int bsr_statistics_init(struct bsr_statistics *self);
uint32_t bsr_statistics_ms_since_last_print(struct bsr_statistics *self);

struct bsr_chnhandler {
    struct bsr_poller *poller;
    void *user_data;
    char addr_inp[ADDR_CAP];
    int state;
    void *sock_inp;
    GList *socks_out;
    struct timespec last_print_ts;
    uint64_t received;
    uint64_t sentok;
    uint64_t eagain;
    uint64_t eagain_multipart;
    // Shared, no need to clean up:
    struct bsr_statistics *stats;
};

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, char *addr_inp,
                         struct bsr_statistics *stats);
ERRT bsr_chnhandler_add_out(struct bsr_chnhandler *self, char *addr);
ERRT bsr_chnhandler_remove_out(struct bsr_chnhandler *self, char *addr);
ERRT bsr_chnhandler_handle_event(struct bsr_chnhandler *self, struct bsr_poller *poller);
