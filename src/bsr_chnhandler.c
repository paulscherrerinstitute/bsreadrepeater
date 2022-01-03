#include <bsr_chnhandler.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
#include <stdio.h>

enum HandlerState {
    RECV = 1,
    SEND = 2,
};

void cleanup_bsr_chnhandler(struct bsr_chnhandler *k) {
    if (k->socket != NULL) {
        int ec = zmq_close(k->socket);
        if (ec != 0) {
            fprintf(stderr, "ERROR cleanup_bsr_chnhandler zmq_close %d\n", ec);
        }
        k->socket = NULL;
    }
}

// TODO merge `bind` and `init`
int bsr_chnhandler_bind(struct bsr_chnhandler *k, void *ctx, int port) {
    int ec;
    k->socket = zmq_socket(ctx, ZMQ_REP);
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

int bsr_chnhandler_init(struct bsr_chnhandler *self, void *poller,
                        void *user_data) {
    int ec;
    self->state = RECV;
    ec = zmq_poller_add(poller, self->socket, user_data, ZMQ_POLLIN);
    if (ec != 0) {
        return ec;
    }
    return 0;
}

int bsr_chnhandler_handle_event(struct bsr_chnhandler *self,
                                struct bsr_poller *poller) {
    int ec;
    size_t const N = 64;
    uint8_t buf[N];
    if (self->state == RECV) {
        int do_recv = 1;
        while (do_recv) {
            int n = zmq_recv(self->socket, buf, N, ZMQ_NOBLOCK);
            if (n == -1) {
                if (errno == EAGAIN) {
                    fprintf(stderr, "no more\n");
                    break;
                } else {
                    fprintf(stderr, "error in recv %d  %s\n", errno,
                            zmq_strerror(errno));
                    return -1;
                }
            } else {
                char sb[64];
                to_hex(sb, buf, n);
                fprintf(stderr, "Received: %d (%s) [%.*s]\n", n, sb, n, buf);
                // TODO only try more if multipart. REQ must be matched by REP.
                do_recv = 0;
                self->state = SEND;
                ec = zmq_poller_modify(poller->poller, self->socket,
                                       ZMQ_POLLOUT);
                // TODO capture zmq error details. caller does not know whether
                // some returned error means that `errno` is set to some valid
                // value, and also not whether `errnor` might point to some
                // zmq-specific error-code.
                if (ec == -1) {
                    fprintf(stderr, "error: %d  %s\n", errno,
                            zmq_strerror(errno));
                };
                NZRET(ec);
            }
        };
    } else if (self->state == SEND) {
        int n = snprintf((char *)buf, N, "reply");
        if (n <= 0) {
            return -1;
        };
        n = zmq_send(self->socket, buf, n, 0);
        if (n == -1) {
            if (errno == EAGAIN) {
                fprintf(stderr, "EAGAIN on send\n");
                return 0;
            } else {
                fprintf(stderr, "ERROR on send %d\n", errno);
                return -1;
            };
        } else {
            self->state = RECV;
            ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLIN);
            NZRET(ec);
        };
    };
    return 0;
}
