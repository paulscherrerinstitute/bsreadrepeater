#pragma once
#include <bsr_timed_events.h>
#include <err.h>
#include <glib.h>
#include <stdint.h>
#include <time.h>

struct bsr_statistics {
    uint64_t received;
    uint64_t sentok;
    uint64_t eagain;
    uint64_t eagain_multipart;
    uint64_t recv_buf_too_small;
    uint64_t recv_bytes;
    uint64_t sent_bytes;
    struct timespec last_print_ts;
    struct timespec datetime_idle_check_last;
    float poll_wait_ema;
    float poll_wait_emv;
    float process_ema;
    float process_emv;
    uint64_t input_reopened;
    struct bsr_timed_events timed_events;
};

ERRT bsr_statistics_init(struct bsr_statistics *self);
uint32_t bsr_statistics_ms_since_last_print(struct bsr_statistics *self);
ERRT bsr_statistics_print_now(struct bsr_statistics *self);
