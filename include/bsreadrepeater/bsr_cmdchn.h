#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <zmq.h>

struct bsr_cmdchn {
    void *socket;
};

void cleanup_bsr_cmdchn(struct bsr_cmdchn *k);
int bsr_cmdchn_bind(struct bsr_cmdchn *k, void *ctx, int port);
int bsr_cmdchn_init(struct bsr_cmdchn *k, void *poller);
int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, void *poller);
