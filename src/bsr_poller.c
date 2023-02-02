/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsreadrepeater/bsr_poller.h>

void cleanup_bsr_poller(struct bsr_poller *k) {
    fprintf(stderr, "cleanup_bsr_poller\n");
    if (k->poller != NULL) {
        int ec = zmq_poller_destroy(&k->poller);
        if (ec != 0) {
            fprintf(stderr, "ERROR zmq_poller_destroy\n");
        }
        k->poller = NULL;
    }
    if (k->timer_poller != NULL) {
        zmq_timers_destroy(&k->timer_poller);
        k->timer_poller = NULL;
    }
}
