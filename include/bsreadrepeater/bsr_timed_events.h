#pragma once
#include <err.h>
#include <stdint.h>
#include <time.h>

enum timed_event_type {
    ENABLE,
};

struct timed_event {
    enum timed_event_type ty;
    // TODO use common MAX
    char addr[80];
};

struct bsr_timed_events {
    void *tree;
    struct timespec ts_init;
};

ERRT bsr_timed_events_init(struct bsr_timed_events *self);
ERRT bsr_timed_events_cleanup(struct bsr_timed_events *self);
ERRT bsr_timed_events_add_input_enable(struct bsr_timed_events *self, char const *addr, uint64_t tsmu);
ERRT bsr_timed_events_most_recent_key(struct bsr_timed_events *self, struct timespec tsnow, uint64_t **retkey,
                                      struct timed_event **ret);
