#pragma once
#include <err.h>
#include <stdint.h>
#include <time.h>

struct bsr_ema {
    float k;
    float ema;
    float emv;
};

ERRT bsr_ema_init(struct bsr_ema *self);
ERRT bsr_ema_update(struct bsr_ema *self, float v);
