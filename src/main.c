#define ZMQ_BUILD_DRAFT_API
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

struct SourceHandler {
    int m1;
};

enum HandlerKind {
    CommandHandler = 1,
    SourceHandler = 2,
};

union HandlerUnion {
    struct CommandHandler cmdh;
    struct SourceHandler srch;
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
            break;
        }
        default: {
            fprintf(stderr, "cleanup_struct_Handler unknown kind %d\n",
                    self->kind);
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

ERRT handlers_handle_msg(struct Handler *self, struct bsr_poller *poller,
                         int *cmdret) {
    int ec;
    if (0) {
        fprintf(stderr, "handlers_handle_msg  kind %d  self %p\n", self->kind,
                (void *)self);
    };
    switch (self->kind) {
    case CommandHandler: {
        struct CommandHandler *h = &self->handler.cmdh;
        ec = bsr_cmdchn_handle_event(&h->cmdchn, poller, cmdret);
        NZRET(ec);
        break;
    }
    case SourceHandler: {
        struct SourceHandler *h = NULL;
        h = h;
        fprintf(stderr, "TODO handlers_handle_msg SourceHandler\n");
        return -1;
        break;
    }
    default: {
        fprintf(stderr, "ERROR handlers_handle_msg unhandled kind %d\n",
                self->kind);
    }
    }
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

    // struct bsr_cmdchn cmdchn __attribute__((cleanup(cleanup_bsr_cmdchn))) =
    // {0};
    // ec = bsr_cmdchn_bind(&cmdchn, ctx, 4242);
    // NZRET(ec);

    struct Handler cmdhandler
        __attribute__((cleanup(cleanup_struct_Handler))) = {
            .kind = CommandHandler,
            .handler =
                {
                    .cmdh = {.cmdchn = {0}},
                },
        };
    ec = bsr_cmdchn_init(&cmdhandler.handler.cmdh.cmdchn, &poller, 4242,
                         &cmdhandler);
    NZRET(ec);
    fprintf(stderr, "bsr_cmdchn_init\n");

    void *timer_tx = zmq_socket(ctx, ZMQ_PUSH);
    void *timer_rx = zmq_socket(ctx, ZMQ_PULL);
    ec = zmq_bind(timer_tx, "ipc:///tmp/4242");
    NZRET(ec);
    ec = zmq_connect(timer_rx, "ipc:///tmp/4242");
    NZRET(ec);

    if (0) {
        ec = raise(SIGINT);
        NZRET(ec);
    }

    void *s1 = zmq_socket(ctx, ZMQ_PUSH);
    void *s2 = zmq_socket(ctx, ZMQ_PULL);
    ec = set_opts(s1);
    NZRET(ec);
    ec = set_opts(s2);
    NZRET(ec);

    ec = zmq_poller_add(poller.poller, s1, NULL, ZMQ_POLLOUT);
    NZRET(ec);
    ec = zmq_poller_add(poller.poller, s2, NULL, ZMQ_POLLIN);
    NZRET(ec);

    GString *addr __attribute__((cleanup(cleanup_g_string))) =
        g_string_new("tcp://127.0.0.1:9155");
    fprintf(stderr, "string created [%s]\n", addr->str);
    ec = zmq_bind(s1, addr->str);
    NZRET(ec);
    ec = zmq_connect(s2, addr->str);
    NZRET(ec);

    int did_send = 0;
    int did_recv = 0;

    int evsmax = 2;
    zmq_poller_event_t evs[2];

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

    fprintf(stderr, "enter loop\n");
    int run_poll_loop = 1;
    for (int i4 = 0; i4 < 100 && run_poll_loop == 1; i4 += 1) {
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
            if (did_send < 1 && ev->socket == s1) {
                fprintf(stderr, "can send\n");
                int n = zmq_send(ev->socket, "123456", 6, 0);
                if (n != 6) {
                    return -1;
                }
                did_send += 1;
            } else if (did_recv < 1 && ev->socket == s2) {
                fprintf(stderr, "can recv\n");
                uint8_t buf[16];
                int n = zmq_recv(ev->socket, buf, 16, 0);
                if (n != 6) {
                    return -1;
                }
                fprintf(stderr, "Received: [%.6s]\n", buf);
                did_recv += 1;
            } else if (ev->user_data != NULL) {
                // fprintf(stderr, "handle via user-data\n");
                int cmdret = 0;
                ec = handlers_handle_msg(ev->user_data, &poller, &cmdret);
                if (ec != 0) {
                    fprintf(stderr, "handle via user-data GOT %d\n", ec);
                };
                NZRET(ec);
                if (cmdret == 1) {
                    fprintf(stderr, "got msg to exit\n");
                    run_poll_loop = 0;
                    break;
                }
            } else {
                fprintf(stderr, "ERROR can not handle event\n");
                return -1;
            }
        }
        if (did_send >= 1) {
            ec = zmq_poller_modify(poller.poller, s1, 0);
            NZRET(ec);
        }
        if (did_recv >= 2) {
            ec = zmq_poller_modify(poller.poller, s2, 0);
            NZRET(ec);
        }
        if (did_send >= 1 && did_recv >= 2) {
            break;
        }
    }
    fprintf(stderr, "Loop done\n");

    ec = zmq_close(timer_tx);
    NZRET(ec);
    ec = zmq_close(timer_rx);
    NZRET(ec);

    ec = zmq_close(s1);
    NZRET(ec);
    ec = zmq_close(s2);
    NZRET(ec);
    return 0;
}
