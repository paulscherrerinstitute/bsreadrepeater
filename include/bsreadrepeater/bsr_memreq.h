#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <zmq.h>

#define CMDMAX 80
#define OUTQ_LEN 12
#define OUTQ_ITEM_LEN 256

struct bsr_memreq {
    char cmdaddr[CMDMAX];
    // poller is just borrowed:
    struct bsr_poller *poller;
    void *user_data;
    void *sock_out;
    int state;
    char outq[OUTQ_LEN][OUTQ_ITEM_LEN];
    char *outq_wp;
    char *outq_rp;
    int outq_len;
};

ERRT cleanup_struct_bsr_memreq(struct bsr_memreq *self);
ERRT bsr_memreq_init(struct bsr_memreq *self, struct bsr_poller *poller, void *user_data, char *cmdaddr);
ERRT bsr_memreq_handle_event(struct bsr_memreq *self, short events);
int bsr_memreq_free_len(struct bsr_memreq *self);
ERRT bsr_memreq_next_cmd_buf(struct bsr_memreq *self, char **buf);
ERRT bsr_memreq_add_cmd(struct bsr_memreq *self, char const *cmd);
