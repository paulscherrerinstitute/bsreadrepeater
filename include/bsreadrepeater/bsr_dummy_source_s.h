#pragma once
#include <bsr_poller.h>

struct bsr_dummy_source {
    // shared:
    struct bsr_poller *poller;
    void *sock;
    int timer_id;
    int timer_in_use;
};
