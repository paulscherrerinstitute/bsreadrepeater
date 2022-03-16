#define ZMQ_BUILD_DRAFT_API
#define _GNU_SOURCE
#include <bsr_ctx1.h>
#include <inttypes.h>
#include <pthread.h>
#include <zmq.h>

static ERRT cleanup_ctx(void **self) {
    int ec;
    if (self != NULL) {
        if (*self != NULL) {
            ec = zmq_ctx_term(*self);
            NZRET(ec);
        }
    }
    return 0;
}

static ERRT cleanup_poller(void **self) {
    int ec;
    if (self != NULL) {
        if (*self != NULL) {
            ec = zmq_poller_destroy(self);
            NZRET(ec);
        }
    }
    return 0;
}

static ERRT cleanup_socket(void **self) {
    int ec;
    if (self != NULL) {
        if (*self != NULL) {
            ec = zmq_close(*self);
            NZRET(ec);
        }
    }
    return 0;
}

static ERRT source(struct ctx1 *self) {
    int ec;
    void *__attribute__((cleanup(cleanup_ctx))) ctx = NULL;
    void *__attribute__((cleanup(cleanup_poller))) poller = NULL;
    void *__attribute__((cleanup(cleanup_socket))) sock1 = NULL;
    ctx = zmq_ctx_new();
    ZMQ_NULLRET(ctx);
    poller = zmq_poller_new();
    ZMQ_NULLRET(poller);
    sock1 = zmq_socket(ctx, ZMQ_PUSH);
    ZMQ_NULLRET(sock1);
    ec = zmq_poller_add(poller, sock1, NULL, ZMQ_POLLOUT);
    ZMQ_NEGONERET(ec);
    int const evsmax = 8;
    zmq_poller_event_t evs[evsmax];
    for (uint64_t i4 = 0;; i4 += 1) {
        int nev = zmq_poller_wait_all(poller, evs, evsmax, 200);
        for (int nix = 0; nix < nev; nix += 1) {
        }
        if (self->stop == 1) {
            break;
        }
    }
    return 0;
}

static void *run_inner(void *self2) {
    struct ctx1 *self = self2;
    fprintf(stderr, "ty %d\n", self->ty);
    switch (self->ty) {
    case 1:
        fprintf(stderr, "source kind\n");
        return (void *)(size_t)source(self);
        break;
    default:
        fprintf(stderr, "unknown kind\n");
    }
    if (0) {
        pthread_exit(NULL);
    }
    return NULL;
}

ERRT ctx1_init(struct ctx1 *self) {
    self->addr = (char *)"tcp://127.0.0.1:32400";
    self->period_ms = 1000;
    self->stop = 0;
    self->thread_started = 0;
    self->ty = 1;
    return 0;
}

ERRT ctx1_start(struct ctx1 *self) {
    int ec;
    ec = pthread_create(&self->thr1, NULL, run_inner, self);
    NZRET(ec);
    self->thread_started = 1;
    ec = pthread_setname_np(self->thr1, "ctx1");
    NZRET(ec);
    return 0;
}

ERRT cleanup_ctx1(struct ctx1 *self) {
    int ec;
    fprintf(stderr, "cleanup_ctx1\n");
    if (self != NULL) {
        self->stop = 1;
        if (self->thread_started == 1) {
            void *retval;
            ec = pthread_join(self->thr1, &retval);
            NZRET(ec);
            fprintf(stderr, "joined ok  retval: %ld\n", (size_t)retval);
            self->thread_started = 0;
        }
    }
    return 0;
}
