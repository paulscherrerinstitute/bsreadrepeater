#include <bsr_sockhelp.h>
#include <zmq.h>

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
    int64_t max_msg = 1024 * 1024 * 8;
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
