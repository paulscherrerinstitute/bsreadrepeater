#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <glib.h>
#include <zmq.h>

struct bsr_chnhandler {
    struct bsr_poller *poller;
    void *user_data;
    int state;
    char *buf;
    int buflen;
    void *sock_inp;
    int in_multipart;
    GList *socks_out;
    struct timespec last_print_ts;
    uint64_t sentok;
    uint64_t eagain;
};

ERRT cleanup_bsr_chnhandler(struct bsr_chnhandler *self);
ERRT bsr_chnhandler_init(struct bsr_chnhandler *self, struct bsr_poller *poller, void *user_data, int port);
ERRT bsr_chnhandler_add_out(struct bsr_chnhandler *self, int port);
ERRT bsr_chnhandler_remove_out(struct bsr_chnhandler *self, int port);
ERRT bsr_chnhandler_handle_event(struct bsr_chnhandler *self, struct bsr_poller *poller);
