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
                fprintf(stderr, "error zmq_ctx_term %d\n", ec);
            }
        }
        ec = zmq_ctx_destroy(*k);
        fprintf(stderr, "error zmq_ctx_destroy %d\n", ec);
        *k = NULL;
    }
}

void to_hex(char *buf, uint8_t *in, int n) {
    char *p1 = buf;
    for (int i = 0; i < n; i += 1) {
        snprintf(p1, 3, "%02x", in[i]);
        p1 += 2;
    }
    *p1 = 0;
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

ERRT command_handler_handle_msg(void) { return 0; }

struct SourceHandler {
    int m1;
};

enum HandlerKind {
    CommandHandler,
    SourceHandler,
};

union HandlerUnion {
    struct CommandHandler cmdh;
    struct SourceHandler srch;
};

struct Handler {
    enum HandlerKind kind;
    union HandlerUnion handler;
};

ERRT handlers_init(struct Handler *self, struct bsr_poller *poller) {
    fprintf(stderr, "handlers_init %d\n", self->kind);
    switch (self->kind) {
    case CommandHandler: {
        struct CommandHandler *h = &self->handler.cmdh;
        fprintf(stderr, "handlers_init CommandHandler\n");
        int ec;
        ec = zmq_poller_add(poller->poller, h->cmdchn.socket, self, ZMQ_POLLIN);
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
    }
    return 0;
}

ERRT handlers_handle_msg(struct Handler *self, struct bsr_poller *poller) {
    fprintf(stderr, "handlers_handle_msg  kind %d  self %p\n", self->kind,
            (void *)self);
    switch (self->kind) {
    case CommandHandler: {
        struct CommandHandler *h = &self->handler.cmdh;
        fprintf(stderr, "handlers_handle_msg CommandHandler\n");
        bsr_cmdchn_handle_event(&h->cmdchn, poller);
        break;
    }
    case SourceHandler: {
        struct SourceHandler *h = NULL;
        h = h;
        fprintf(stderr, "todo SourceHandler\n");
        return -1;
        break;
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
    poller.poller = zmq_poller_new();
    fprintf(stderr, "poller %p\n", poller.poller);

    struct bsr_cmdchn cmdchn __attribute__((cleanup(cleanup_bsr_cmdchn))) = {0};
    ec = bsr_cmdchn_bind(&cmdchn, ctx, 4242);
    NZRET(ec);

    struct Handler cmdhandler = {
        .kind = CommandHandler,
        .handler =
            {
                .cmdh = {.cmdchn = cmdchn},
            },
    };
    ec = handlers_init(&cmdhandler, &poller);
    NZRET(ec);

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
    for (int i4 = 0; i4 < 16; i4 += 1) {
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
                fprintf(stderr, "user-data\n");
                ec = handlers_handle_msg(ev->user_data, &poller);
                NZRET(ec);
            }
            if (0 && ev->socket == cmdchn.socket &&
                (ev->events & ZMQ_POLLIN) != 0) {
                fprintf(stderr, "cmdchn POLLIN\n");
                uint8_t buf[16];
                int n = zmq_recv(ev->socket, buf, 16, 0);
                if (n == -1) {
                    fprintf(stderr, "ERROR cmd recv %d\n", n);
                    return -1;
                }
                char sb[64];
                to_hex(sb, buf, n);
                fprintf(stderr, "Received: %d (%s) [%.*s]\n", n, sb, n, buf);
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
