#define ZMQ_BUILD_DRAFT_API
#include <bsr_cmdchn.h>
#include <bsr_poller.h>
#include <bsr_rep.h>
#include <bsr_stats.h>
#include <bsr_str.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <zmq.h>

size_t const MAXSTR = 128;

static void cleanup_zmq_context(void **self) {
    int ec;
    if (*self != NULL) {
        if (0) {
            ec = zmq_ctx_term(*self);
            if (ec != 0) {
                fprintf(stderr, "ERROR zmq_ctx_term %d\n", ec);
            }
        }
        ec = zmq_ctx_destroy(*self);
        if (ec != 0) {
            fprintf(stderr, "ERROR zmq_ctx_destroy %d\n", ec);
        }
        *self = NULL;
    }
}

ERRT cleanup_bsrep(struct bsrep *self) {
    fprintf(stderr, "cleanup_bsrep\n");
    // TODO make sure we join the thread here.
    return 0;
}

ERRT bsrep_init(struct bsrep *self, char *cmd_addr, char *startup_file) {
    if (strnlen(cmd_addr, MAXSTR) >= MAXSTR) {
        return 1;
    }
    if (strnlen(startup_file, MAXSTR) >= MAXSTR) {
        return 1;
    }
    self->cmd_addr = malloc(MAXSTR);
    self->startup_file = malloc(MAXSTR);
    strncpy(self->cmd_addr, cmd_addr, MAXSTR);
    return 0;
}

ERRT bsrep_run(struct bsrep *self) {
    int ec = 0;
    struct bsr_statistics stats = {0};
    bsr_statistics_init(&stats);

    void *ctx __attribute__((cleanup(cleanup_zmq_context))) = zmq_ctx_new();
    NULLRET(ctx);

    if (0) {
        ec = zmq_ctx_set(ctx, ZMQ_IPV6, 1);
        NZRET(ec);
    }

    struct bsr_poller poller __attribute__((cleanup(cleanup_bsr_poller))) = {0};
    poller.ctx = ctx;
    poller.poller = zmq_poller_new();

    struct HandlerList handler_list __attribute((cleanup(cleanup_struct_HandlerList))) = {0};
    ec = handler_list_init(&handler_list);
    NZRET(ec);

    struct Handler cmdhandler __attribute__((cleanup(cleanup_struct_Handler))) = {0};
    cmdhandler.kind = CommandHandler;
    ec = bsr_cmdchn_init(&cmdhandler.handler.cmdh.cmdchn, &poller, &cmdhandler, self->cmd_addr, &handler_list, &stats);
    NZRET(ec);

    int stf = -1;
    if (self->startup_file != NULL) {
        stf = open(self->startup_file, O_RDONLY);
    }
    if (stf != -1) {
        struct Handler *hh __attribute__((cleanup(cleanup_free_struct_Handler))) = NULL;
        hh = malloc(sizeof(struct Handler));
        NULLRET(hh);
        hh->kind = StartupHandler;
        ec = bsr_startupcmd_init(&hh->handler.start, &poller, hh, stf, self->cmd_addr);
        NZRET(ec);
        handler_list_add(&handler_list, hh);
        hh = NULL;
    }

    int const evsmax = 64;
    zmq_poller_event_t evs[evsmax];
    int run_poll_loop = 1;
    for (uint64_t i4 = 0; run_poll_loop == 1; i4 += 1) {
        struct timespec ts1;
        struct timespec ts2;
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        int nev = zmq_poller_wait_all(poller.poller, evs, evsmax, 10000);
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        {
            float const k = 0.05f;
            int64_t dt = time_delta_mu(ts1, ts2);
            float d1 = (float)dt - stats.poll_wait_ema;
            stats.poll_wait_ema += k * d1;
            stats.poll_wait_emv = (1.0f - k) * (stats.poll_wait_emv + k * d1 * d1);
            if (FALSE) {
                fprintf(stderr, "d1 %7.0f  dt %8ld  ema %5.0f  emv %6.0f\n", d1, dt, stats.poll_wait_ema,
                        sqrtf(stats.poll_wait_emv));
            }
        };
        if (nev == -1) {
            if (errno == EAGAIN) {
                fprintf(stderr, "Nothing to do\n");
            } else {
                fprintf(stderr, "Poller error: %s\n", zmq_strerror(errno));
                return -1;
            }
        }
        for (int i = 0; i < nev; i += 1) {
            zmq_poller_event_t *ev = evs + i;
            if (ev->user_data != NULL) {
                struct ReceivedCommand cmd = {0};
                ec = handlers_handle_msg(ev->user_data, &poller, &cmd, ts2);
                if (ec != 0) {
                    fprintf(stderr, "handle via user-data GOT %d\n", ec);
                };
                NZRET(ec);
                if (cmd.ty == CmdExit) {
                    fprintf(stderr, "Exit command received\n");
                    run_poll_loop = 0;
                    break;
                };
            } else {
                fprintf(stderr, "ERROR can not handle event\n");
                return -1;
            }
        }
        {
            // Gather statistics on required time to process the events.
            struct timespec ts3;
            clock_gettime(CLOCK_MONOTONIC, &ts3);
            float const k = 0.05f;
            int64_t dt = time_delta_mu(ts2, ts3);
            float d1 = (float)dt - stats.process_ema;
            stats.process_ema += k * d1;
            stats.process_emv = (1.0f - k) * (stats.process_emv + k * d1 * d1);
            if (FALSE) {
                fprintf(stderr, "d1 %7.0f  dt %8ld  ema %5.0f  emv %6.0f\n", d1, dt, stats.process_ema,
                        sqrtf(stats.process_emv));
            }
        }
    }
    fprintf(stderr, "Cleaning up...\n");
    return 0;
}
