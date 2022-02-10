#pragma once
#include <err.h>

ERRT cleanup_zmq_ctx(void **self);
ERRT cleanup_zmq_socket(void **self);
ERRT set_basic_sock_opts(void *sock);
ERRT set_linger(void *sock, int linger);
ERRT set_rcvhwm(void *sock, int rcvhwm);
ERRT set_sndhwm(void *sock, int sndhwm);
ERRT set_rcvbuf(void *sock, int rcvbuf);
ERRT set_sndbuf(void *sock, int sndbuf);
ERRT req_rep_single(char const *addr, char const *msg, int n);
