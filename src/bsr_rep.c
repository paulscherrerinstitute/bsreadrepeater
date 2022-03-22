#define ZMQ_BUILD_DRAFT_API
#define _GNU_SOURCE
#include <bsr_cmdchn.h>
#include <bsr_poller.h>
#include <bsr_rep.h>
#include <bsr_sockhelp.h>
#include <bsr_stats.h>
#include <bsr_str.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <zmq.h>

uint32_t const GLOBAL_STATS_PRINT_DT_MS = 4000;
int64_t const IDLE_CHECK_DT_MS = 30000;
int64_t const MAX_IDLE_MS = 120000;
size_t const MAXSTR = 128;

ERRT cleanup_bsrep(struct bsrep *self) {
    int ec;
    fprintf(stderr, "cleanup_bsrep\n");
    if (self->thread_created == 1) {
        self->stop = 1;
        // TODO use a throw-away context to send the exit message.
        void *thrret = NULL;
        ec = pthread_join(self->thr1, &thrret);
        if (ec != 0) {
            fprintf(stderr, "can not join bsrep thread\n");
            return ec;
        }
        self->thr1 = 0;
        self->thread_created = 0;
    }
    if (self->cmd_addr != NULL) {
        free(self->cmd_addr);
        self->cmd_addr = NULL;
    }
    if (self->startup_file != NULL) {
        free(self->startup_file);
        self->startup_file = NULL;
    }
    return 0;
}

ERRT bsrep_init(struct bsrep *self, char const *cmd_addr, char *startup_file) {
    self->inactive_check_next_ix = 0;
    clock_gettime(CLOCK_MONOTONIC, &self->ts_inactive_check_last);
    if (strnlen(cmd_addr, MAXSTR) >= MAXSTR) {
        return 1;
    }
    self->cmd_addr = malloc(MAXSTR);
    strncpy(self->cmd_addr, cmd_addr, MAXSTR);
    if (startup_file != NULL) {
        if (strnlen(startup_file, MAXSTR) >= MAXSTR) {
            return 1;
        }
        self->startup_file = malloc(MAXSTR);
        strncpy(self->startup_file, startup_file, MAXSTR);
    }
    return 0;
}

static ERRT remove_and_add_source(struct bsr_memreq *memreq, struct bsr_chnhandler *h2) {
    int ec;
    int outputs_count = bsr_chnhandler_outputs_count(h2);
    fprintf(stderr, "outputs_count %d\n", outputs_count);
    // TODO is there enough space in memreq buffer for all commands?
    int nfree = bsr_memreq_free_len(memreq);
    if (nfree < 2 + outputs_count) {
        // Memory buffer does not have enough space for the required commands.
        // Have to try again later.
        fprintf(stderr, "not enough space in memreq for idle commands\n");
    } else {
        char *dst = NULL;
        ec = bsr_memreq_next_cmd_buf(memreq, &dst);
        NZRET(ec);
        snprintf(dst, OUTQ_ITEM_LEN, "remove-source,%s", h2->addr_inp);
        ec = bsr_memreq_next_cmd_buf(memreq, &dst);
        NZRET(ec);
        snprintf(dst, OUTQ_ITEM_LEN, "add-source,%s", h2->addr_inp);
        GList *sol = h2->socks_out;
        while (sol != NULL) {
            struct sockout *so = (struct sockout *)sol->data;
            ec = bsr_memreq_next_cmd_buf(memreq, &dst);
            NZRET(ec);
            snprintf(dst, OUTQ_ITEM_LEN, "add-output,%s,%s", h2->addr_inp, so->addr);
            sol = sol->next;
        }
    }
    return 0;
}

static ERRT idle_source_check(struct bsrep *self, struct HandlerList *handler_list, struct bsr_memreq *memreq,
                              struct timespec tsnow) {
    int ec;
    clock_gettime(CLOCK_REALTIME, &self->stats.datetime_idle_check_last);
    uint64_t checked_idle = 0;
    uint64_t found_idle = 0;
    uint64_t ix = 0;
    GList *p1 = handler_list->list;
    while (p1 != NULL && ix < self->inactive_check_next_ix) {
        if (p1->next == NULL) {
            self->inactive_check_next_ix = 0;
            p1 = handler_list->list;
            break;
        } else {
            ix += 1;
            p1 = p1->next;
        }
    }
    while (p1 != NULL) {
        struct Handler *h = handler_list_get_data(p1);
        if (h->kind == SourceHandler) {
            struct bsr_chnhandler *h2 = &h->handler.src;
            int64_t dt = time_delta_mu(h2->ts_recv_last, tsnow);
            if (dt > MAX_IDLE_MS * 1000) {
                h2->ts_recv_last = tsnow;
                if (FALSE) {
                    ec = remove_and_add_source(memreq, h2);
                    NZRET(ec);
                }
                if (TRUE) {
                    ec = bsr_chnhandler_reopen_input(h2);
                    NZRET(ec);
                }
                found_idle += 1;
            }
            checked_idle += 1;
        }
        if (p1->next == NULL) {
            self->inactive_check_next_ix = 0;
            p1 = handler_list->list;
            break;
        } else {
            self->inactive_check_next_ix += 1;
            p1 = p1->next;
        }
        if (found_idle >= 1) {
            break;
        }
    }
    if (found_idle != 0) {
        fprintf(stderr, "idle checked  checked_idle %" PRIu64 "  found_idle %" PRIu64 "\n", checked_idle, found_idle);
    }
    if (found_idle != 0) {
    } else {
        self->ts_inactive_check_last = tsnow;
    }
    return 0;
}

static ERRT bsrep_run_inner(struct bsrep *self) {
    int ec;
    bsr_statistics_init(&self->stats);

    void *ctx __attribute__((cleanup(cleanup_zmq_ctx))) = zmq_ctx_new();
    NULLRET(ctx);
    if (TRUE) {
        ec = zmq_ctx_set(ctx, ZMQ_IPV6, 1);
        NZRET(ec);
    }
    if (TRUE) {
        ec = zmq_ctx_set(ctx, ZMQ_IO_THREADS, 1);
        NZRET(ec);
    }
    if (TRUE) {
        ec = zmq_ctx_set(ctx, ZMQ_MAX_SOCKETS, 1024 * 4);
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
    ec = bsr_cmdchn_init(&cmdhandler.handler.cmdh.cmdchn, &poller, &cmdhandler, self->cmd_addr, &handler_list,
                         &self->stats);
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
        ec = handler_list_add(&handler_list, hh);
        NZRET(ec);
        hh = NULL;
    }

    // Shared ref:
    struct bsr_memreq *g_memreq = NULL;
    {
        struct Handler *hh __attribute__((cleanup(cleanup_free_struct_Handler))) = NULL;
        hh = malloc(sizeof(struct Handler));
        NULLRET(hh);
        hh->kind = MemReqHandler;
        ec = bsr_memreq_init(&hh->handler.memreq, &poller, hh, self->cmd_addr);
        NZRET(ec);
        ec = handler_list_add(&handler_list, hh);
        NZRET(ec);
        g_memreq = &hh->handler.memreq;
        hh = NULL;
    }

    int const evsmax = 64;
    zmq_poller_event_t evs[evsmax];
    for (uint64_t i4 = 0;; i4 += 1) {
        if (self->do_polls_max > 0 && self->polls_count >= self->do_polls_max) {
            break;
        }
        if (self->stop == 1) {
            if (self->polls_count >= self->do_polls_min) {
                break;
            } else {
            }
        }
        struct timespec ts1;
        struct timespec ts2;
        clock_gettime(CLOCK_MONOTONIC, &ts1);
        int nev = zmq_poller_wait_all(poller.poller, evs, evsmax, 200);
        if (FALSE) {
            fprintf(stderr, "TRACE  poll nev %d\n", nev);
        }
        self->polls_count += 1;
        clock_gettime(CLOCK_MONOTONIC, &ts2);
        {
            struct bsr_statistics *stats = &self->stats;
            float const k = 0.05f;
            int64_t dt = time_delta_mu(ts1, ts2);
            float d1 = (float)dt - stats->poll_wait_ema;
            stats->poll_wait_ema += k * d1;
            stats->poll_wait_emv = (1.0f - k) * (stats->poll_wait_emv + k * d1 * d1);
            if (FALSE) {
                fprintf(stderr, "d1 %7.0f  dt %8ld  ema %5.0f  emv %6.0f\n", d1, dt, stats->poll_wait_ema,
                        sqrtf(stats->poll_wait_emv));
            }
        }
        if (nev == -1) {
            if (errno == EAGAIN) {
                if (FALSE) {
                    fprintf(stderr, "Nothing to do\n");
                }
            } else {
                fprintf(stderr, "Poller error: %s\n", zmq_strerror(errno));
                return -1;
            }
        }
        for (int i = 0; i < nev; i += 1) {
            zmq_poller_event_t *ev = evs + i;
            if (ev->user_data != NULL) {
                struct ReceivedCommand cmd = {0};
                ec = handlers_handle_msg(ev->user_data, ev->events, &poller, &cmd, ts2);
                if (ec != 0) {
                    fprintf(stderr, "handle via user-data GOT %d\n", ec);
                }
                NZRET(ec);
                if (cmd.ty == CmdExit) {
                    fprintf(stderr, "Exit command received\n");
                    self->stop = 1;
                    break;
                }
            } else {
                fprintf(stderr, "ERROR can not handle event\n");
                return -1;
            }
        }
        struct timespec ts3;
        clock_gettime(CLOCK_MONOTONIC, &ts3);
        {
            // Gather statistics on required time to process the events.
            struct bsr_statistics *stats = &self->stats;
            float const k = 0.05f;
            int64_t dt = time_delta_mu(ts2, ts3);
            float d1 = (float)dt - stats->process_ema;
            stats->process_ema += k * d1;
            stats->process_emv = (1.0f - k) * (stats->process_emv + k * d1 * d1);
            if (FALSE) {
                fprintf(stderr, "d1 %7.0f  dt %8ld  ema %5.0f  emv %6.0f\n", d1, dt, stats->process_ema,
                        sqrtf(stats->process_emv));
            }
        }
        if (FALSE && time_delta_mu(self->ts_inactive_check_last, ts3) > IDLE_CHECK_DT_MS * 1000) {
            ec = idle_source_check(self, &handler_list, g_memreq, ts3);
            NZRET(ec);
        }
        if (bsr_statistics_ms_since_last_print(&self->stats) > GLOBAL_STATS_PRINT_DT_MS) {
            bsr_statistics_print_now(&self->stats);
        }
    }
    return 0;
}

static void *bsrep_run_inner_thr(void *selfptr) {
    struct bsrep *self = selfptr;
    int ec = bsrep_run_inner(self);
    self->thrret = ec;
    return selfptr;
}

ERRT bsrep_start(struct bsrep *self) {
    int ec;
    ec = pthread_create(&self->thr1, NULL, bsrep_run_inner_thr, self);
    NZRET(ec);
    self->thread_created = 1;
    ec = pthread_setname_np(self->thr1, "bsrep");
    NZRET(ec);
    return 0;
}

ERRT bsrep_wait_for_done(struct bsrep *self) {
    int ec;
    if (self->thread_created == 1) {
        ec = pthread_join(self->thr1, NULL);
        self->thread_created = 0;
        NZRET(ec);
        return 0;
    } else {
        return 1;
    }
}
