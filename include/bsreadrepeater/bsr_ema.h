#pragma once
#include <err.h>
#include <stdint.h>
#include <time.h>

struct bsr_ema {
    float k;
    float ema;
    float emv;
    uint64_t update_count;
};

ERRT bsr_ema_init(struct bsr_ema *self);
ERRT bsr_ema_update(struct bsr_ema *self, float v);

struct bsr_ema_ext {
    float k;
    float ema;
    float emv;
    float min;
    float max;
    uint64_t update_count;
};

ERRT bsr_ema_ext_init(struct bsr_ema_ext *self);
ERRT bsr_ema_ext_update(struct bsr_ema_ext *self, float v);
