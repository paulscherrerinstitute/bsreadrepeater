#pragma once
#include <bsr_timed_events.h>
#include <err.h>
#include <stdint.h>
#include <sys/types.h>
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
    uint64_t evs_max;
    uint64_t poll_evs_total;
};

ERRT bsr_statistics_init(struct bsr_statistics *self);
ERRT bsr_statistics_cleanup(struct bsr_statistics *self);
uint32_t bsr_statistics_ms_since_last_print(struct bsr_statistics *self);
ERRT bsr_statistics_print_now(struct bsr_statistics *self);

struct stats_source_out_pub {
    char addr[64];
    uint64_t sent_count;
    uint64_t sent_bytes;
    uint64_t eagain;
    uint64_t eagain_multipart;
};

struct stats_source_pub {
    uint64_t received;
    uint64_t mhparsed;
    uint64_t dhparsed;
    uint64_t json_parse_errors;
    uint64_t dhdecompr;
    uint64_t data_header_lz4_count;
    uint64_t data_header_bslz4_count;
    uint64_t sentok;
    uint64_t recv_bytes;
    uint64_t sent_bytes;
    uint64_t bsread_errors;
    uint64_t input_reopened;
    uint64_t throttling_enable_count;
    uint64_t timestamp_out_of_range_count;
    uint64_t unexpected_frames_count;
    float mpmsglen_ema;
    float mpmsglen_emv;
    float msg_dt_ema;
    float msg_dt_emv;
    float msg_emit_age_ema;
    float msg_emit_age_emv;
    float msg_emit_age_min;
    float msg_emit_age_max;
    uint out_count;
    struct stats_source_out_pub *out_stats;
};
