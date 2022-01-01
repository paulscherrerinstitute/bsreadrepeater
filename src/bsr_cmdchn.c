#include <bsr_cmdchn.h>
#include <errno.h>
#include <stdio.h>

void cleanup_bsr_cmdchn(struct bsr_cmdchn *k) {
    if (k->socket != NULL) {
        int ec = zmq_close(k->socket);
        if (ec != 0) {
            fprintf(stderr, "ERROR cleanup_bsr_cmdchn zmq_close %d\n", ec);
        }
        k->socket = NULL;
    }
}

int bsr_cmdchn_bind(struct bsr_cmdchn *k, void *ctx, int port) {
    int ec;
    k->socket = zmq_socket(ctx, ZMQ_STREAM);
    if (k->socket == NULL) {
        return -1;
    }
    char buf[32];
    snprintf(buf, 32, "tcp://127.0.0.1:%d", port);
    ec = zmq_bind(k->socket, buf);
    if (ec != 0) {
        return ec;
    }
    return 0;
}

int bsr_cmdchn_init(struct bsr_cmdchn *self, void *poller) {
    int ec;
    ec = zmq_poller_add(poller, self->socket, self, ZMQ_POLLIN);
    if (ec != 0) {
        return ec;
    }
    return 0;
}

int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, void *poller) {
    poller = poller;
    size_t const N = 32;
    uint8_t buf[N];
    int do_recv = 1;
    while (do_recv) {
        fprintf(stderr, "bsr_cmdchn_handle_event try recv socket %p\n",
                self->socket);
        int n = zmq_recv(self->socket, buf, N, ZMQ_NOBLOCK);
        if (n < 0) {
            if (errno == EAGAIN) {
                fprintf(stderr, "no more\n");
                break;
            } else {
                fprintf(stderr, "error in recv %d\n", n);
                return -1;
            }
        }
    };
    return 0;
}
