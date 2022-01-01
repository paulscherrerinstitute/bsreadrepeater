#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <err.h>
#include <zmq.h>

struct bsr_poller {
    void *poller;
};

void cleanup_bsr_poller(struct bsr_poller *k);
