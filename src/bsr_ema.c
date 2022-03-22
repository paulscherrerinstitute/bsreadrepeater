#include <bsr_ema.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

ERRT bsr_ema_init(struct bsr_ema *self) {
    self->k = 0.05;
    self->ema = 0;
    self->emv = 0;
    return 0;
}

ERRT bsr_ema_update(struct bsr_ema *self, float v) {
    float k = self->k;
    float dv = v - self->ema;
    self->ema += k * dv;
    self->emv = (1 - k) * (self->emv + k * dv * dv);
    if (0) {
        fprintf(stderr, "v % 7.3f  dv % 8.5f  ema % 5.0f  emv % 6.0f\n", v, dv, self->ema, sqrtf(self->emv));
    }
    return 0;
}
