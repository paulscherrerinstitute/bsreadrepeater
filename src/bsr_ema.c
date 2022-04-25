#include <bsr_ema.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

ERRT bsr_ema_init(struct bsr_ema *self) {
    self->k = 0.05;
    self->ema = 0;
    self->emv = 0;
    self->update_count = 0;
    return 0;
}

ERRT bsr_ema_update(struct bsr_ema *self, float v) {
    self->update_count += 1;
    float k = self->k;
    float dv = v - self->ema;
    self->ema += k * dv;
    self->emv = (1 - k) * (self->emv + k * dv * dv);
    if (0) {
        fprintf(stderr, "v % 7.3f  dv % 8.5f  ema % 5.0f  emv % 6.0f\n", v, dv, self->ema, sqrtf(self->emv));
    }
    return 0;
}

ERRT bsr_ema_ext_init(struct bsr_ema_ext *self) {
    self->k = 0.05;
    self->ema = 0;
    self->emv = 0;
    self->min = INFINITY;
    self->max = -INFINITY;
    self->update_count = 0;
    return 0;
}

ERRT bsr_ema_ext_update(struct bsr_ema_ext *self, float v) {
    self->update_count += 1;
    float k = self->k;
    float dv = v - self->ema;
    self->ema += k * dv;
    self->emv = (1 - k) * (self->emv + k * dv * dv);
    if (v < self->min) {
        self->min = v;
    }
    if (v > self->max) {
        self->max = v;
    }
    return 0;
}
