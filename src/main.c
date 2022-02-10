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
#include <bsr_rep.h>
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

int main_inner(int const argc, char const *const *argv) {
    int ec;
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

    char *startup_file = NULL;
    if (argc >= 3) {
        startup_file = malloc(strlen(argv[2]));
        strcpy(startup_file, argv[2]);
        fprintf(stderr, "Read startup commands from: %s\n", startup_file);
    }

    struct bsrep __attribute__((cleanup(cleanup_bsrep))) rep = {0};
    ec = bsrep_init(&rep, cmd_addr, startup_file);
    NZRET(ec);

    ec = bsrep_run(&rep);
    NZRET(ec);

    return 0;
}

int main(int const argc, char const *const *argv) {
    {
        // For shared libs, make sure that we got dynamically linked against a compatible version.
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
