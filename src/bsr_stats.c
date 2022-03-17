#include <bsr_stats.h>
#include <inttypes.h>
#include <stdio.h>

int bsr_statistics_init(struct bsr_statistics *self) {
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    self->received = 0;
    self->sentok = 0;
    self->eagain = 0;
    self->eagain_multipart = 0;
    self->recv_buf_too_small = 0;
    self->poll_wait_ema = 0.0;
    self->poll_wait_emv = 0.0;
    self->input_reopened = 0;
    return 0;
}

uint32_t bsr_statistics_ms_since_last_print(struct bsr_statistics *self) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    struct timespec t1 = self->last_print_ts;
    uint64_t dt =
        (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - (uint64_t)(t1.tv_sec * 1000 + t1.tv_nsec / 1000000);
    if (dt > UINT32_MAX) {
        dt = UINT32_MAX;
    }
    return dt;
}

ERRT bsr_statistics_print_now(struct bsr_statistics *self) {
    struct bsr_statistics *st = self;
    fprintf(stderr,
            "received %" PRIu64 "  sent %" PRIu64 "  busy %" PRIu64 "  busy-in-mp %" PRIu64
            "  recv_buf_too_small %" PRIu64 "  recv bytes %" PRIu64 "  sent bytes %" PRIu64 "\n",
            st->received, st->sentok, st->eagain, st->eagain_multipart, st->recv_buf_too_small, st->recv_bytes,
            st->sent_bytes);
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    return 0;
}
