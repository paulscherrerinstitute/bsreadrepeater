#include <bsreadrepeater/bsr_poller.h>

void cleanup_bsr_poller(struct bsr_poller *k) {
    if (k->poller != NULL) {
        int ec = zmq_poller_destroy(&k->poller);
        if (ec != 0) {
            fprintf(stderr, "ERROR zmq_poller_destroy\n");
        }
        k->poller = NULL;
    }
}
