#include <bsr_memreq.h>
#include <string.h>
#include <unistd.h>

ERRT cleanup_struct_bsr_memreq(struct bsr_memreq *self) {
    int ec;
    if (self != NULL) {
        if (self->sock_out != NULL) {
            if (self->poller != NULL) {
                ec = zmq_poller_remove(self->poller->poller, self->sock_out);
                if (ec == -1) {
                    fprintf(stderr, "ERROR  cleanup_struct_bsr_memreq  zmq_poller_remove\n");
                }
            }
            ec = zmq_close(self->sock_out);
            if (ec == -1) {
                fprintf(stderr, "ERROR  cleanup_struct_bsr_memreq  zmq_close\n");
            }
            self->sock_out = NULL;
        }
    }
    return 0;
}

ERRT bsr_memreq_init(struct bsr_memreq *self, struct bsr_poller *poller, void *user_data, char *cmdaddr) {
    int ec;
    self->state = 0;
    self->outq_len = 0;
    self->outq_wp = &self->outq[0][0];
    self->outq_rp = &self->outq[0][0];
    self->poller = poller;
    self->user_data = user_data;
    if (strnlen(cmdaddr, CMDMAX) >= CMDMAX) {
        fprintf(stderr, "cmdaddr too long\n");
        return -1;
    }
    strncpy(self->cmdaddr, cmdaddr, CMDMAX - 1);
    self->sock_out = zmq_socket(poller->ctx, ZMQ_REQ);
    ZMQ_NULLRET(self->sock_out);
    ec = zmq_connect(self->sock_out, self->cmdaddr);
    ZMQ_NEGONERET(ec);
    ec = zmq_poller_add(poller->poller, self->sock_out, user_data, 0);
    ZMQ_NEGONERET(ec);
    return 0;
}

static ERRT update_poller(struct bsr_memreq *self) {
    int ec;
    void *poller = self->poller->poller;
    void *sock = self->sock_out;
    short flags = 0;
    if (self->state == 1) {
        flags = ZMQ_POLLIN;
        ec = zmq_poller_modify(poller, sock, flags);
        ZMQ_NEGONERET(ec);
    } else if (self->outq_len > 0) {
        flags = ZMQ_POLLOUT;
        ec = zmq_poller_modify(poller, sock, flags);
        ZMQ_NEGONERET(ec);
    }
    return 0;
}

ERRT bsr_memreq_handle_event(struct bsr_memreq *self, short events) {
    fprintf(stderr, "bsr_memreq_handle_event\n");
    int ec;
    if (self->state == 0) {
        if ((events && ZMQ_POLLOUT) == 0) {
            fprintf(stderr, "memreq state send, but output not ready\n");
            return -1;
        }
        if (self->outq_len == 0) {
            fprintf(stderr, "memreq state send, but nothing to send\n");
            return -1;
        }
        ec = zmq_send(self->sock_out, self->outq_rp, strnlen(self->outq_rp, OUTQ_ITEM_LEN), ZMQ_DONTWAIT);
        ZMQ_NEGONERET(ec);
        self->outq_rp += OUTQ_ITEM_LEN;
        if (self->outq_rp >= &self->outq[0][0] + OUTQ_LEN * OUTQ_ITEM_LEN) {
            self->outq_rp = &self->outq[0][0];
        }
        self->outq_len -= 1;
        self->state = 1;
        ec = update_poller(self);
        NZRET(ec);
    } else if (self->state == 1) {
        if ((events && ZMQ_POLLIN) == 0) {
            fprintf(stderr, "memreq state recv, but input not ready\n");
            return -1;
        }
        char gg[64];
        int n = zmq_recv(self->sock_out, gg, 64, ZMQ_DONTWAIT);
        if (n == -1) {
            ZMQ_NEGONERET(n);
        }
        self->state = 0;
        ec = update_poller(self);
        NZRET(ec);
    } else {
        fprintf(stderr, "memreq unknown state %d\n", self->state);
        return -1;
    }
    return 0;
}

int bsr_memreq_free_len(struct bsr_memreq *self) { return OUTQ_LEN - self->outq_len; }

ERRT bsr_memreq_next_cmd_buf(struct bsr_memreq *self, char **buf) {
    if (bsr_memreq_free_len(self) < 1) {
        fprintf(stderr, "memreq outq full\n");
        return -1;
    }
    *buf = self->outq_wp;
    self->outq_wp += OUTQ_ITEM_LEN;
    if (self->outq_wp >= &self->outq[0][0] + OUTQ_LEN * OUTQ_ITEM_LEN) {
        self->outq_wp = &self->outq[0][0];
    }
    self->outq_len += 1;
    int ec;
    ec = update_poller(self);
    NZRET(ec);
    return 0;
}

ERRT bsr_memreq_add_cmd(struct bsr_memreq *self, char const *cmd) {
    if (bsr_memreq_free_len(self) < 1) {
        fprintf(stderr, "memreq outq full\n");
        return -1;
    }
    if (strnlen(cmd, OUTQ_ITEM_LEN) >= OUTQ_ITEM_LEN) {
        fprintf(stderr, "memreq command too long\n");
        return -1;
    }
    strncpy(self->outq_wp, cmd, OUTQ_ITEM_LEN);
    self->outq_wp += OUTQ_ITEM_LEN;
    if (self->outq_wp >= &self->outq[0][0] + OUTQ_LEN * OUTQ_ITEM_LEN) {
        self->outq_wp = &self->outq[0][0];
    }
    self->outq_len += 1;
    int ec;
    ec = update_poller(self);
    NZRET(ec);
    return 0;
}
