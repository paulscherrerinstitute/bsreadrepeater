/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <err.h>
#include <zmq.h>

struct bsr_poller {
    void *poller;
    void *ctx;
    void *timer_poller;
};

void cleanup_bsr_poller(struct bsr_poller *k);
