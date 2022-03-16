#pragma once
#include <err.h>

struct ctx1 {
    unsigned long thr1;
    int thread_started;
    _Atomic int stop;
    int ty;
    char *addr;
    int period_ms;
};

ERRT ctx1_init(struct ctx1 *ctx1);
ERRT ctx1_start(struct ctx1 *ctx1);
ERRT cleanup_ctx1(struct ctx1 *self);
