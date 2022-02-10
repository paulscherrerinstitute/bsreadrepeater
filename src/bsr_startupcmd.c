/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_startupcmd.h>
#include <unistd.h>

ERRT cleanup_struct_bsr_startupcmd(struct bsr_startupcmd *self) {
    if (self != NULL) {
        self->poller = NULL;
        if (self->sock_out != NULL) {
            zmq_close(self->sock_out);
        };
    };
    return 0;
}

ERRT bsr_startupcmd_init(struct bsr_startupcmd *self, struct bsr_poller *poller, void *user_data, int fd,
                         char *cmdaddr) {
    int ec;
    self->poller = poller;
    self->user_data = user_data;
    self->fd = fd;
    strncpy(self->cmdaddr, cmdaddr, 80);
    self->cmdaddr[80 - 1] = 0;
    self->wp = self->buf;
    self->rp = self->buf;
    self->ep = self->buf + 256;
    self->state = 0;
    self->sock_out = zmq_socket(poller->ctx, ZMQ_REQ);
    ZMQ_NULLRET(self->sock_out);
    ec = zmq_connect(self->sock_out, self->cmdaddr);
    ZMQ_NEGONERET(ec);
    ec = zmq_poller_add(poller->poller, self->sock_out, user_data, 0);
    ZMQ_NEGONERET(ec);
    ec = zmq_poller_add_fd(poller->poller, self->fd, user_data, ZMQ_POLLIN);
    ZMQ_NEGONERET(ec);
    return 0;
}

ERRT bsr_startupcmd_handle_event(struct bsr_startupcmd *self) {
    int ec;
    if (self->state == 0) {
        ssize_t n = read(self->fd, self->wp, self->ep - self->wp);
        if (n == -1) {
            zmq_poller_remove_fd(self->poller->poller, self->fd);
            self->fd = -1;
            return -1;
        } else if (n == 0) {
            zmq_poller_remove_fd(self->poller->poller, self->fd);
            close(self->fd);
            self->fd = -1;
        } else {
            self->wp += n;
            self->state = 1;
            ec = zmq_poller_modify_fd(self->poller->poller, self->fd, 0);
            ZMQ_NEGONERET(ec);
            ec = zmq_poller_modify(self->poller->poller, self->sock_out, ZMQ_POLLOUT);
            ZMQ_NEGONERET(ec);
        };
    } else if (self->state == 1) {
        int evs = 0;
        size_t evsl = sizeof(int);
        ec = zmq_getsockopt(self->sock_out, ZMQ_EVENTS, &evs, &evsl);
        ZMQ_NEGONERET(ec);
        if ((evs & ZMQ_POLLOUT) != 0) {
            int nfound = 0;
            if (self->wp > self->rp) {
                char *p1 = self->rp;
                while (p1 < self->wp) {
                    if (*p1 == '\n') {
                        *p1 = 0;
                        p1 += 1;
                        ec = zmq_send(self->sock_out, self->rp, strnlen(self->rp, 256), ZMQ_DONTWAIT);
                        ZMQ_NEGONERET(ec);
                        memmove(self->buf, p1, self->wp - p1);
                        self->wp = self->buf + (self->wp - p1);
                        self->rp = self->buf;
                        p1 = self->rp;
                        nfound += 1;
                        break;
                    } else {
                        p1 += 1;
                    };
                };
            } else {
                // Nothing else to do here.
            };
            if (nfound == 0) {
                if (self->fd == -1) {
                    // if nothing parsed and file closed, remove from poller and close.
                    ec = zmq_poller_remove(self->poller->poller, self->sock_out);
                    ZMQ_NEGONERET(ec);
                } else {
                    if (self->wp < self->ep) {
                        self->state = 0;
                        ec = zmq_poller_modify(self->poller->poller, self->sock_out, 0);
                        ZMQ_NEGONERET(ec);
                        ec = zmq_poller_modify_fd(self->poller->poller, self->fd, ZMQ_POLLIN);
                        ZMQ_NEGONERET(ec);
                    } else {
                        fprintf(stderr, "ERROR command line too long\n");
                        ec = zmq_poller_remove(self->poller->poller, self->sock_out);
                        ZMQ_NEGONERET(ec);
                        ec = zmq_poller_remove_fd(self->poller->poller, self->fd);
                        ZMQ_NEGONERET(ec);
                    };
                };
            } else {
                // Need to receive reply first.
                self->state = 2;
                ec = zmq_poller_modify(self->poller->poller, self->sock_out, ZMQ_POLLIN);
                ZMQ_NEGONERET(ec);
            };
        };
    } else if (self->state == 2) {
        char gg[64];
        int n = zmq_recv(self->sock_out, gg, 64, ZMQ_DONTWAIT);
        if (n == -1) {
            ZMQ_NEGONERET(n);
        } else {
            ec = zmq_poller_modify(self->poller->poller, self->sock_out, ZMQ_POLLOUT);
            ZMQ_NEGONERET(ec);
            self->state = 1;
        };
    };
    return 0;
}
