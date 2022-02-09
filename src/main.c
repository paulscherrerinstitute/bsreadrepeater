/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#define ZMQ_BUILD_DRAFT_API
#include <bsr_chnhandler.h>
#include <bsr_cmdchn.h>
#include <bsr_ctx1.h>
#include <bsr_poller.h>
#include <bsr_startupcmd.h>
#include <bsr_str.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>

void cleanup_g_string(GString **k) {
    if (*k != NULL) {
        g_string_free(*k, TRUE);
        *k = NULL;
    }
}

void cleanup_zmq_context(void **k) {
    int ec;
    if (*k != NULL) {
        if (0) {
            ec = zmq_ctx_term(*k);
            if (ec != 0) {
                fprintf(stderr, "ERROR zmq_ctx_term %d\n", ec);
            }
        }
        ec = zmq_ctx_destroy(*k);
        if (ec != 0) {
            fprintf(stderr, "ERROR zmq_ctx_destroy %d\n", ec);
        }
        *k = NULL;
    }
}

// Common options for sockets.
ERRT set_opts(void *sock) {
    int ec;
    int linger = 800;
    ec = zmq_setsockopt(sock, ZMQ_LINGER, &linger, sizeof(int));
    NZRET(ec);
    int ipv6 = 1;
    ec = zmq_setsockopt(sock, ZMQ_IPV6, &ipv6, sizeof(int));
    NZRET(ec);
    int64_t max_msg = 1024 * 256;
    ec = zmq_setsockopt(sock, ZMQ_MAXMSGSIZE, &max_msg, sizeof(int64_t));
    NZRET(ec);
    int rcvhwm = 200;
    ec = zmq_setsockopt(sock, ZMQ_RCVHWM, &rcvhwm, sizeof(int));
    NZRET(ec);
    int sndhwm = 200;
    ec = zmq_setsockopt(sock, ZMQ_SNDHWM, &sndhwm, sizeof(int));
    NZRET(ec);
    return 0;
}

guint hhf(gconstpointer p) { return (guint)(size_t)p; }

gboolean heq(gconstpointer a, gconstpointer b) {
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return +1;
    }
    return 0;
}

void cleanup_struct_Handler(struct Handler *self) {
    int ec;
    if (self != NULL && self->kind != 0) {
        switch (self->kind) {
        case CommandHandler: {
            cleanup_bsr_cmdchn(&self->handler.cmdh.cmdchn);
            break;
        }
        case SourceHandler: {
            ec = cleanup_bsr_chnhandler(&self->handler.src);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup SourceHandler\n");
            }
            break;
        }
        case StartupHandler: {
            ec = cleanup_struct_bsr_startupcmd(&self->handler.start);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup StartupHandler\n");
            }
            break;
        }
        default: {
            fprintf(stderr, "cleanup_struct_Handler unknown kind %d\n", self->kind);
        }
        }
        self->kind = 0;
    } else {
        fprintf(stderr, "WARN cleanup_struct_Handler nothing to do??\n");
    };
}

ERRT handlers_init(struct Handler *self, struct bsr_poller *poller) {
    poller = poller;
    fprintf(stderr, "handlers_init REMOVE\n");
    exit(-1);
    fprintf(stderr, "handlers_init %d\n", self->kind);
    switch (self->kind) {
    case CommandHandler: {
        fprintf(stderr, "ERROR handlers_init CommandHandler NOT DONE HERE\n");
        break;
    }
    case SourceHandler: {
        struct SourceHandler *h = NULL;
        h = h;
        fprintf(stderr, "todo SourceHandler\n");
        return -1;
        break;
    }
    default: {
        fprintf(stderr, "handlers_init unknown kind %d\n", self->kind);
    }
    }
    return 0;
}

ERRT handlers_handle_msg(struct Handler *self, struct bsr_poller *poller, struct ReceivedCommand *cmd,
                         struct timespec tspoll) {
    int ec;
    if (0) {
        fprintf(stderr, "handlers_handle_msg  kind %d  self %p\n", self->kind, (void *)self);
    };
    switch (self->kind) {
    case CommandHandler: {
        struct CommandHandler *h = &self->handler.cmdh;
        ec = bsr_cmdchn_handle_event(&h->cmdchn, poller, cmd);
        NZRET(ec);
        break;
    }
    case SourceHandler: {
        ec = bsr_chnhandler_handle_event(&self->handler.src, poller, tspoll);
        NZRET(ec);
        break;
    }
    case StartupHandler: {
        ec = bsr_startupcmd_handle_event(&self->handler.start);
        NZRET(ec);
        break;
    }
    default: {
        fprintf(stderr, "ERROR handlers_handle_msg unhandled kind %d\n", self->kind);
    }
    }
    return 0;
}

ERRT cleanup_struct_HandlerList(struct HandlerList *self) {
    GList *it = self->list;
    while (it != NULL) {
        cleanup_struct_Handler(it->data);
        // TODO this assumes that items in this list are always on the heap.
        free(it->data);
        it->data = NULL;
        it = it->next;
    };
    g_list_free(self->list);
    self->list = NULL;
    return -1;
}

ERRT handler_list_init(struct HandlerList *self) {
    self->list = NULL;
    return 0;
}

ERRT handler_list_add(struct HandlerList *self, struct Handler *handler) {
    // TODO currently we assume that `handler` is on the heap and we assume ownership.
    self->list = g_list_append(self->list, handler);
    return 0;
}

ERRT handler_list_remove(struct HandlerList *self, struct Handler *handler) {
    cleanup_struct_Handler(handler);
    self->list = g_list_remove(self->list, handler);
    // TODO check that HandlerList owns the contained struct Handler.
    free(handler);
    return 0;
}

struct Handler *handler_list_find_by_input_addr_2(struct HandlerList *self, char const *addr) {
    GList *it = self->list;
    while (it != NULL) {
        struct Handler *handler = it->data;
        if (handler->kind == SourceHandler) {
            if (strncmp(handler->handler.src.addr_inp, addr, ADDR_CAP) == 0) {
                return handler;
            };
        };
        it = it->next;
    }
    return NULL;
}

struct bsr_chnhandler *handler_list_find_by_input_addr(struct HandlerList *self, char const *addr) {
    GList *it = self->list;
    while (it != NULL) {
        struct Handler *handler = it->data;
        if (handler->kind == SourceHandler) {
            if (strncmp(handler->handler.src.addr_inp, addr, ADDR_CAP) == 0) {
                return &handler->handler.src;
            };
        };
        it = it->next;
    }
    return NULL;
}

void cleanup_socket(void **k) {
    if (*k != NULL) {
        zmq_close(*k);
        *k = NULL;
    }
}

void cleanup_free_struct_Handler(struct Handler **k) {
    if (*k != NULL) {
        free(*k);
        *k = NULL;
    }
}

int main_inner(int argc, char **argv) {
    int ec;
    {
        int maj, min, pat;
        zmq_version(&maj, &min, &pat);
        if (maj != 4 || min < 3) {
            fprintf(stderr, "ERROR require libzmq version 4.3 or compatible\n");
            return 1;
        };
        if (zmq_has("draft") != 1) {
            fprintf(stderr, "ERROR libzmq has no support for zmq_poller\n");
            return 1;
        };
    }
    int const cmd_addr_max = 64;
    char cmd_addr[cmd_addr_max];
    strncpy(cmd_addr, "tcp://127.0.0.1:4242", cmd_addr_max);
    if (argc < 2) {
        fprintf(stderr, "Usage: bsrep <COMMAND_SOCKET_ADDRESS> [STARTUP_COMMAND_FILE]\n\n");
        return 0;
    }
    if (argc >= 2) {
        strncpy(cmd_addr, argv[1], cmd_addr_max);
        cmd_addr[cmd_addr_max - 1] = 0;
    }
    fprintf(stderr, "Command socket address: %s\n", cmd_addr);

    char const *startup_file = NULL;
    if (argc >= 3) {
        startup_file = argv[2];
        fprintf(stderr, "Read startup commands from: %s\n", startup_file);
    }

    struct bsr_statistics stats = {0};
    bsr_statistics_init(&stats);

    void *ctx __attribute__((cleanup(cleanup_zmq_context))) = zmq_ctx_new();
    NULLRET(ctx);
    // ec = zmq_ctx_set(ctx, ZMQ_IPV6, 1);

    struct bsr_poller poller __attribute__((cleanup(cleanup_bsr_poller))) = {0};
    poller.ctx = ctx;
    poller.poller = zmq_poller_new();

    struct HandlerList handler_list __attribute((cleanup(cleanup_struct_HandlerList))) = {0};
    ec = handler_list_init(&handler_list);
    NZRET(ec);

    struct Handler cmdhandler __attribute__((cleanup(cleanup_struct_Handler))) = {0};
    cmdhandler.kind = CommandHandler;
    ec = bsr_cmdchn_init(&cmdhandler.handler.cmdh.cmdchn, &poller, &cmdhandler, cmd_addr, &handler_list, &stats);
    NZRET(ec);

    int stf = -1;
    if (startup_file != NULL) {
        stf = open(startup_file, O_RDONLY);
    }
    if (stf != -1) {
        struct Handler *hh __attribute__((cleanup(cleanup_free_struct_Handler))) = NULL;
        hh = malloc(sizeof(struct Handler));
        NULLRET(hh);
        hh->kind = StartupHandler;
        ec = bsr_startupcmd_init(&hh->handler.start, &poller, hh, stf, cmd_addr);
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
            };
        };
        if (nev == -1) {
            if (errno == EAGAIN) {
                fprintf(stderr, "Nothing to do\n");
            } else {
                fprintf(stderr, "Poller error: %s\n", zmq_strerror(errno));
                return -1;
            };
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
            };
        };
        {
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
            };
        };
    };
    fprintf(stderr, "Cleaning up...\n");
    return 0;
}

int main(int argc, char **argv) {
    struct ctx1 __attribute__((cleanup(cleanup_ctx1))) ctx1 = {0};
    struct ctx1_args args = {0};
    args.ty = 1;
    ctx1_run(&ctx1, &args);
    return 0;
    //
    fprintf(stderr, "bsrep 0.1.3\n");
    int x = main_inner(argc, argv);
    fprintf(stderr, "bsrep exit(%d)\n", x);
    return x;
}
