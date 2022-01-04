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

struct CommandHandler {
    struct bsr_cmdchn cmdchn;
};

enum HandlerKind {
    CommandHandler = 1,
    SourceHandler = 2,
};

union HandlerUnion {
    struct CommandHandler cmdh;
    struct bsr_chnhandler src;
};

struct Handler {
    enum HandlerKind kind;
    union HandlerUnion handler;
};

void cleanup_struct_Handler(struct Handler *self) {
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
    }
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

struct HandlerList {
    GArray *list;
};

ERRT cleanup_struct_HandlerList(struct HandlerList *self) {
    fprintf(stderr, "TODO cleanup_struct_HandlerList\n");
    self = self;
    return -1;
}

ERRT handler_list_init(struct HandlerList *self) {
    self->list = g_array_new(FALSE, FALSE, sizeof(struct Handler));
    return 0;
}

ERRT handler_list_add(struct HandlerList *self, struct Handler handler) {
    g_array_append_vals(self->list, &handler, 1);
    return 0;
}

int main(int argc, char **argv) {
    argc = argc;
    argv = argv;
    int ec;

    void *ctx __attribute__((cleanup(cleanup_zmq_context))) = zmq_ctx_new();
    NULLRET(ctx);

    struct bsr_poller poller __attribute__((cleanup(cleanup_bsr_poller))) = {0};
    poller.ctx = ctx;
    poller.poller = zmq_poller_new();

    struct Handler cmdhandler __attribute__((cleanup(cleanup_struct_Handler))) = {0};
    cmdhandler.kind = CommandHandler;
    ec = bsr_cmdchn_init(&cmdhandler.handler.cmdh.cmdchn, &poller, &cmdhandler, 4242);
    NZRET(ec);

    struct Handler chn1 __attribute__((cleanup(cleanup_struct_Handler))) = {0};
    chn1.kind = SourceHandler;
    ec = bsr_chnhandler_init(&chn1.handler.src, &poller, &chn1, 50000);
    NZRET(ec);

    struct HandlerList handler_list __attribute((cleanup(cleanup_struct_HandlerList))) = {0};
    ec = handler_list_init(&handler_list);
    NZRET(ec);

    if (0) {
        ec = raise(SIGINT);
        NZRET(ec);
    }

    int const evsmax = 8;
    zmq_poller_event_t evs[evsmax];

    {
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
                    fprintf(stderr, "got msg to exit\n");
                    run_poll_loop = 0;
                    break;
                } else if (cmd.ty == CmdAdd) {
                    fprintf(stderr, "add output\n");
                    ec = bsr_chnhandler_add_out(&chn1.handler.src, 50001);
                    NZRET(ec);
                } else if (cmd.ty == CmdRemove) {
                    fprintf(stderr, "remove output\n");
                    ec = bsr_chnhandler_remove_out(&chn1.handler.src, 50001);
                    NZRET(ec);
                };
            } else {
                fprintf(stderr, "ERROR can not handle event\n");
                return -1;
            }
        }
    }
    fprintf(stderr, "Exit\n");
    return 0;
}
