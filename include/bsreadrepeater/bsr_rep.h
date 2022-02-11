#pragma once
#include <bsr_stats.h>
#include <err.h>
#include <stdint.h>

struct bsrep {
    char *cmd_addr;
    char *startup_file;
    int thread_created;
    unsigned long thr1;
    _Atomic int stop;
    int thrret;
    uint64_t do_polls_min;
    uint64_t do_polls_max;
    uint64_t polls_count;
    struct bsr_statistics stats;
};

ERRT cleanup_bsrep(struct bsrep *self);
ERRT bsrep_init(struct bsrep *self, char const *cmd_addr, char *startup_file);
ERRT bsrep_start(struct bsrep *self);
ERRT bsrep_wait_for_done(struct bsrep *self);
