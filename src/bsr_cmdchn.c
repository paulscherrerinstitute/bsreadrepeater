#include <bsr_cmdchn.h>
#include <err.h>
#include <errno.h>
#include <hex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum HandlerState {
    RECV = 1,
    SEND = 2,
};

void cleanup_bsr_cmdchn(struct bsr_cmdchn *k) {
    if (k->socket != NULL) {
        int ec = zmq_close(k->socket);
        if (ec != 0) {
            fprintf(stderr, "ERROR cleanup_bsr_cmdchn zmq_close %d\n", ec);
        }
        k->socket = NULL;
    }
}

int bsr_cmdchn_init(struct bsr_cmdchn *self, struct bsr_poller *poller, void *user_data, int port) {
    int ec;
    self->state = RECV;
    // self->socket = zmq_socket(poller->ctx, ZMQ_STREAM);
    self->socket = zmq_socket(poller->ctx, ZMQ_REP);
    if (self->socket == NULL) {
        fprintf(stderr, "can not create socket: %s\n", zmq_strerror(errno));
        return -1;
    }
    char buf[32];
    snprintf(buf, 32, "tcp://127.0.0.1:%d", port);
    ec = zmq_bind(self->socket, buf);
    if (ec != 0) {
        fprintf(stderr, "can not bind socket: %s\n", zmq_strerror(errno));
        return ec;
    }
    ec = zmq_poller_add(poller->poller, self->socket, user_data, ZMQ_POLLIN);
    if (ec != 0) {
        fprintf(stderr, "can not add to poller: %s\n", zmq_strerror(errno));
        return ec;
    }
    return 0;
}

int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, struct bsr_poller *poller, struct ReceivedCommand *cmd) {
    int ec;
    size_t const N = 64;
    uint8_t buf[N];
    if (self->state == RECV) {
        int do_recv = 1;
        while (do_recv) {
            int n = zmq_recv(self->socket, buf, N, ZMQ_DONTWAIT);
            if (n == -1) {
                if (errno == EAGAIN) {
                    fprintf(stderr, "no more\n");
                    break;
                } else {
                    fprintf(stderr, "error in recv %d  %s\n", errno, zmq_strerror(errno));
                    return -1;
                }
            } else {
                char sb[64];
                to_hex(sb, buf, n);
                fprintf(stderr, "Received: %d (%s) [%.*s]\n", n, sb, n, buf);
                if (n == 4 && memcmp("exit", buf, n) == 0) {
                    cmd->ty = CmdExit;
                } else if (n >= 5 && memcmp("add", buf, 3) == 0) {
                    cmd->ty = CmdAdd;
                    char *p2 = (char *)buf + n;
                    errno = 0;
                    int port = strtol((char *)buf + 4, &p2, 10);
                    fprintf(stderr, "port %d\n", port);
                    cmd->var.add.port = port;
                } else if (n == 6 && memcmp("remove", buf, n) == 0) {
                    cmd->ty = CmdRemove;
                };
                // TODO only try more if multipart. REQ must be matched by
                // REP.
                do_recv = 0;
                self->state = SEND;
                ec = zmq_poller_modify(poller->poller, self->socket, ZMQ_POLLOUT);
                // TODO capture zmq error details. caller does not know whether
                // some returned error means that `errno` is set to some valid
                // value, and also not whether `errnor` might point to some
                // zmq-specific error-code.
                if (ec == -1) {
                    fprintf(stderr, "error: %d  %s\n", errno, zmq_strerror(errno));
                };
                NZRET(ec);
            }
        };
    } else if (self->state == SEND) {
        int n = snprintf((char *)buf, N, "reply");
        if (n <= 0) {
            return -1;
        };
        n = zmq_send(self->socket, buf, n, ZMQ_DONTWAIT);
        if (n == -1) {
            if (errno == EAGAIN) {
                // TODO should not happen with REQ/REP.
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
