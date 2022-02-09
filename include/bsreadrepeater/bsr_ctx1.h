#pragma once
#include <err.h>

struct ctx1 {
    unsigned long thr1;
    _Atomic int stop;
};

struct ctx1_args {
    int ty;
    struct ctx1 *ctx1;
};

ERRT ctx1_run(struct ctx1 *ctx1, struct ctx1_args *args);
ERRT cleanup_ctx1(struct ctx1 *self);
