#define ZMQ_BUILD_DRAFT_API
#include <bsr_chnhandler.h>
#include <bsr_cmdchn.h>
#include <bsr_poller.h>
#include <bsreadrepeater.h>
#include <err.h>
#include <errno.h>
#include <glib.h>
#include <signal.h>
#include <stdatomic.h>
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
    // TODO remove output
    fprintf(stderr, "cleanup_struct_Handler\n");
    int ec;
    if (self != NULL && self->kind != 0) {
        switch (self->kind) {
        case CommandHandler: {
            struct bsr_cmdchn *h = &self->handler.cmdh.cmdchn;
            ec = zmq_close(h->socket);
            if (ec == -1) {
                fprintf(stderr, "ERROR can not close bsr_cmdchn socket\n");
            }
            break;
        }
        case SourceHandler: {
            ec = cleanup_bsr_chnhandler(&self->handler.src);
            if (ec == -1) {
                fprintf(stderr, "ERROR cleanup SourceHandler\n");
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
    fprintf(stderr, "handlers_init REMOVE\n");
    exit(-1);
    fprintf(stderr, "handlers_init %d\n", self->kind);
    switch (self->kind) {
    case CommandHandler: {
        struct CommandHandler *h = &self->handler.cmdh;
        h = h;
        fprintf(stderr, "handlers_init CommandHandler\n");
        int ec;
        poller = poller;
        // ec = bsr_cmdchn_init(&h->cmdchn, poller->poller, self);
        NZRET(ec);
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

ERRT handlers_handle_msg(struct Handler *self, struct bsr_poller *poller, struct ReceivedCommand *cmd) {
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
        ec = bsr_chnhandler_handle_event(&self->handler.src, poller);
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
    self->list = g_list_append(self->list, handler);
    return 0;
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

int main2(int argc, char **argv) {
    argc = argc;
    argv = argv;
    int ec;

    void *ctx __attribute__((cleanup(cleanup_zmq_context))) = zmq_ctx_new();
    NULLRET(ctx);

    struct bsr_poller poller __attribute__((cleanup(cleanup_bsr_poller))) = {0};
    poller.ctx = ctx;
    poller.poller = zmq_poller_new();

    struct HandlerList handler_list __attribute((cleanup(cleanup_struct_HandlerList))) = {0};
    ec = handler_list_init(&handler_list);
    NZRET(ec);

    struct Handler cmdhandler __attribute__((cleanup(cleanup_struct_Handler))) = {0};
    cmdhandler.kind = CommandHandler;
    ec = bsr_cmdchn_init(&cmdhandler.handler.cmdh.cmdchn, &poller, &cmdhandler, 4242, &handler_list);
    NZRET(ec);

    if (0) {
        ec = raise(SIGINT);
        NZRET(ec);
    }

    int const evsmax = 8;
    zmq_poller_event_t evs[evsmax];

    if (0) {
        GHashTable *ht = g_hash_table_new(hhf, heq);
        if (ht == NULL) {
            fprintf(stderr, "can not create hash table\n");
            return -1;
        }
        gpointer p1 = (void *)123;
        gboolean a;
        a = g_hash_table_add(ht, p1);
        if (a == FALSE) {
            fprintf(stderr, "insert A\n");
            return -1;
        }
        a = g_hash_table_add(ht, p1);
        if (a == FALSE) {
            fprintf(stderr, "insert B\n");
            return -1;
        }
    }

    int run_poll_loop = 1;
    for (int i4 = 0; i4 < 100000 && run_poll_loop == 1; i4 += 1) {
        if (0) {
            fprintf(stderr, "polling...\n");
        };
        int nev = zmq_poller_wait_all(poller.poller, evs, evsmax, 3000);
        if (nev == -1) {
            if (errno == EAGAIN) {
                fprintf(stderr, "Poller EAGAIN\n");
            } else {
                fprintf(stderr, "Poller error: %d\n", nev);
                return -1;
            }
        }
        for (int i = 0; i < nev; i += 1) {
            zmq_poller_event_t *ev = evs + i;
            if (ev->user_data != NULL) {
                // fprintf(stderr, "handle via user-data\n");
                struct ReceivedCommand cmd = {0};
                ec = handlers_handle_msg(ev->user_data, &poller, &cmd);
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
    }
    fprintf(stderr, "Cleaning up...\n");
    return 0;
}

int main(int argc, char **argv) {
    int x = main2(argc, argv);
    fprintf(stderr, "bsrep exit(%d)\n", x);
    return x;
}
