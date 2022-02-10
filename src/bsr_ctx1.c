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
    void *__attribute__((cleanup(cleanup_ctx))) ctx = NULL;
    void *__attribute__((cleanup(cleanup_poller))) poller = NULL;
    void *__attribute__((cleanup(cleanup_socket))) sock1 = NULL;
    ctx = zmq_ctx_new();
    ZMQ_NULLRET(ctx);
    poller = zmq_poller_new();
    ZMQ_NULLRET(poller);
    sock1 = zmq_socket(ctx, ZMQ_PUSH);
    ZMQ_NULLRET(sock1);
    int const evsmax = 8;
    zmq_poller_event_t evs[evsmax];
    int run_poll_loop = 1;
    for (uint64_t i4 = 0; run_poll_loop == 1; i4 += 1) {
        int nev = zmq_poller_wait_all(poller, evs, evsmax, 200);
        nev = nev;
        if (self->stop == 1) {
            break;
        }
    }
    return 0;
}

static void *sub(void *args2) {
    struct ctx1_args *args = args2;
    fprintf(stderr, "ty %d\n", args->ty);
    switch (args->ty) {
    case 1:
        fprintf(stderr, "source kind\n");
        return (void *)(size_t)source(args->ctx1);
        break;
    default:
        fprintf(stderr, "unknown kind\n");
    }
    pthread_exit(NULL);
    return NULL;
}

ERRT ctx1_start(struct ctx1 *ctx1, struct ctx1_args *args) {
    int ec;
    args->ctx1 = ctx1;
    ec = pthread_create(&ctx1->thr1, NULL, sub, args);
    NZRET(ec);
    ctx1->thread_started = 1;
    ec = pthread_setname_np(ctx1->thr1, "ctx1");
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
