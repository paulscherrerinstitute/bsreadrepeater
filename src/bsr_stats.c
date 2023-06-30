#include <bsr_stats.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>

ERRT bsr_statistics_init(struct bsr_statistics *self) {
    int ec;
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    self->received = 0;
    self->sentok = 0;
    self->eagain = 0;
    self->eagain_multipart = 0;
    self->recv_buf_too_small = 0;
    self->poll_wait_ema = 0.0;
    self->poll_wait_emv = 0.0;
    self->input_reopened = 0;
    self->evs_max = 0;
    self->poll_evs_total = 0;
    // TODO add cleanup logic. Currently only used once.
    ec = bsr_timed_events_init(&self->timed_events);
    NZRET(ec);
    return 0;
}

ERRT bsr_statistics_cleanup(struct bsr_statistics *self) {
    bsr_timed_events_cleanup(&self->timed_events);
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
            "recvm %" PRIu64 "  sendm %" PRIu64 "  busy %" PRIu64 "  busymp %" PRIu64 "  recv_buf_too_small %" PRIu64
            "  recvb %" PRIu64 "  sendb %" PRIu64 "  evsmax %" PRIu64
            "  pollwa %5.0f  pollwv %6.0f  pollevstot %" PRIu64 "\n",
            st->received, st->sentok, st->eagain, st->eagain_multipart, st->recv_buf_too_small, st->recv_bytes,
            st->sent_bytes, self->evs_max, st->poll_wait_ema, sqrtf(st->poll_wait_emv), self->poll_evs_total);
    clock_gettime(CLOCK_MONOTONIC, &self->last_print_ts);
    return 0;
}
