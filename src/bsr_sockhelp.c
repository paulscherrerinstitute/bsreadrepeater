#include <bsr_sockhelp.h>
#include <err.h>
#include <zmq.h>

static size_t MAX_MSG_SIZE = 1024 * 1024 * 20;

ERRT cleanup_zmq_ctx(void **self) {
    int ec;
    if (self != NULL) {
        if (*self != NULL) {
            ec = zmq_ctx_term(*self);
            if (ec != 0) {
                fprintf(stderr, "ERROR zmq_ctx_term %d\n", ec);
            }
            *self = NULL;
        }
    }
    return 0;
}

ERRT cleanup_zmq_socket(void **self) {
    int ec;
    if (self != NULL) {
        if (*self != NULL) {
            ec = zmq_close(*self);
            if (ec != 0) {
                fprintf(stderr, "ERROR zmq_close %d\n", ec);
            }
            *self = NULL;
        }
    }
    return 0;
}

ERRT set_basic_sock_opts(void *sock) {
    int ec;
    int linger = 800;
    ec = zmq_setsockopt(sock, ZMQ_LINGER, &linger, sizeof(int));
    NZRET(ec);
    int ipv6 = 1;
    ec = zmq_setsockopt(sock, ZMQ_IPV6, &ipv6, sizeof(int));
    NZRET(ec);
    // TODO choose max size depending on (expected) channel types.
    // We can't be sure about the channels that a source is supposed to
    // contain, so this requires domain knowledge.
    int64_t max_msg = MAX_MSG_SIZE;
    ec = zmq_setsockopt(sock, ZMQ_MAXMSGSIZE, &max_msg, sizeof(int64_t));
    NZRET(ec);
    return 0;
}

ERRT set_linger(void *sock, int linger) {
    int ec;
    int val = linger;
    int len = sizeof(val);
    ec = zmq_setsockopt(sock, ZMQ_LINGER, &val, len);
    if (ec == -1) {
        return -1;
    }
    return 0;
}

ERRT set_rcvhwm(void *sock, int rcvhwm) {
    int ec;
    int len = sizeof(rcvhwm);
    ec = zmq_setsockopt(sock, ZMQ_RCVHWM, &rcvhwm, len);
    if (ec == -1) {
        return -1;
    }
    return 0;
}

ERRT set_sndhwm(void *sock, int sndhwm) {
    int ec;
    int len = sizeof(sndhwm);
    ec = zmq_setsockopt(sock, ZMQ_SNDHWM, &sndhwm, len);
    if (ec == -1) {
        return -1;
    }
    return 0;
}

ERRT set_rcvbuf(void *sock, int rcvbuf) {
    int ec;
    int len = sizeof(rcvbuf);
    ec = zmq_setsockopt(sock, ZMQ_RCVBUF, &rcvbuf, len);
    if (ec == -1) {
        return -1;
    }
    return 0;
}

ERRT set_sndbuf(void *sock, int sndbuf) {
    int ec;
    int len = sizeof(sndbuf);
    ec = zmq_setsockopt(sock, ZMQ_SNDBUF, &sndbuf, len);
    if (ec == -1) {
        return -1;
    }
    return 0;
}

static ERRT set_keepalive(void *sock) {
    int ec;
    int val;
    int len = sizeof(val);
    val = 1;
    ec = zmq_setsockopt(sock, ZMQ_TCP_KEEPALIVE, &val, len);
    ZMQ_NEGONERET(ec);
    val = 120;
    ec = zmq_setsockopt(sock, ZMQ_TCP_KEEPALIVE_IDLE, &val, len);
    ZMQ_NEGONERET(ec);
    val = 30;
    ec = zmq_setsockopt(sock, ZMQ_TCP_KEEPALIVE_INTVL, &val, len);
    ZMQ_NEGONERET(ec);
    val = 4;
    ec = zmq_setsockopt(sock, ZMQ_TCP_KEEPALIVE_CNT, &val, len);
    ZMQ_NEGONERET(ec);
    val = 10000;
    ec = zmq_setsockopt(sock, ZMQ_HEARTBEAT_IVL, &val, len);
    ZMQ_NEGONERET(ec);
    val = 60000;
    ec = zmq_setsockopt(sock, ZMQ_HEARTBEAT_TIMEOUT, &val, len);
    ZMQ_NEGONERET(ec);
    val = 60000;
    ec = zmq_setsockopt(sock, ZMQ_HEARTBEAT_TTL, &val, len);
    ZMQ_NEGONERET(ec);
    return 0;
}

ERRT set_pull_sock_opts(void *sock, int hwm, int buf) {
    int ec;
    ec = set_basic_sock_opts(sock);
    NZRET(ec);
    ec = set_keepalive(sock);
    NZRET(ec);
    ec = set_rcvhwm(sock, hwm);
    NZRET(ec);
    ec = set_rcvbuf(sock, buf);
    NZRET(ec);
    return 0;
}

ERRT set_push_sock_opts(void *sock, int hwm, int buf) {
    int ec;
    ec = set_basic_sock_opts(sock);
    NZRET(ec);
    ec = set_keepalive(sock);
    NZRET(ec);
    ec = set_sndhwm(sock, hwm);
    NZRET(ec);
    ec = set_sndbuf(sock, buf);
    NZRET(ec);
    return 0;
}

ERRT req_rep_single(char const *addr, char const *msg, int n) {
    int ec;
    void *ctx __attribute__((cleanup(cleanup_zmq_ctx))) = NULL;
    ctx = zmq_ctx_new();
    ZMQ_NULLRET(ctx);
    void *sock __attribute__((cleanup(cleanup_zmq_socket))) = NULL;
    sock = zmq_socket(ctx, ZMQ_REQ);
    ZMQ_NULLRET(ctx);
    {
        int timeout = 1000;
        ec = zmq_setsockopt(sock, ZMQ_RCVTIMEO, &timeout, sizeof(int));
        NZRET(ec);
    }
    ec = zmq_connect(sock, addr);
    ZMQ_NEGONERET(ec);
    ec = zmq_send(sock, msg, n, 0);
    ZMQ_NEGONERET(ec);
    char buf[64];
    ec = zmq_recv(sock, buf, sizeof(buf), 0);
    ZMQ_NEGONERET(ec);
    fprintf(stderr, "req_rep_single DONE\n");
    return 0;
}
