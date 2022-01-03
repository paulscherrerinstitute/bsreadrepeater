#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <zmq.h>

struct bsr_chnhandler {
    void *socket;
    int state;
};

void cleanup_bsr_chnhandler(struct bsr_chnhandler *k);
int bsr_chnhandler_bind(struct bsr_chnhandler *k, void *ctx, int port);
int bsr_chnhandler_init(struct bsr_chnhandler *k, void *poller,
                        void *user_data);
int bsr_chnhandler_handle_event(struct bsr_chnhandler *self,
                                struct bsr_poller *poller);
