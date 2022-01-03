#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <zmq.h>

struct bsr_cmdchn {
    void *socket;
    int state;
};

void cleanup_bsr_cmdchn(struct bsr_cmdchn *k);
int bsr_cmdchn_init(struct bsr_cmdchn *k, struct bsr_poller *poller, int port,
                    void *user_data);
int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, struct bsr_poller *poller,
                            int *cmdret);
