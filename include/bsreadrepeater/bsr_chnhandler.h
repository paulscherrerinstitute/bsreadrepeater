#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <glib.h>
#include <zmq.h>

#define ADDR_CAP 80

struct bsr_chnhandler {
    struct bsr_poller *poller;
    void *user_data;
    char addr_inp[ADDR_CAP];
    int state;
    char *buf;
    int buflen;
    void *sock_inp;
    GList *socks_out;
    struct timespec last_print_ts;
    uint64_t sentok;
    uint64_t eagain;
    uint64_t eagain_multipart;
};

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, char *addr_inp);
ERRT bsr_chnhandler_add_out(struct bsr_chnhandler *self, char *addr);
ERRT bsr_chnhandler_remove_out(struct bsr_chnhandler *self, char *addr);
ERRT bsr_chnhandler_handle_event(struct bsr_chnhandler *self, struct bsr_poller *poller);
