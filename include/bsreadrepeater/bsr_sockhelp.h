#pragma once
#include <err.h>

ERRT cleanup_zmq_ctx(void **self);
ERRT cleanup_zmq_socket(void **self);
ERRT set_basic_sock_opts(void *sock);
ERRT set_linger(void *sock, int linger);
ERRT set_msgmax(void *sock, int msgmax);
ERRT set_rcvhwm(void *sock, int rcvhwm);
ERRT set_sndhwm(void *sock, int sndhwm);
ERRT set_rcvbuf(void *sock, int rcvbuf);
ERRT set_sndbuf(void *sock, int sndbuf);
ERRT set_pull_sock_opts(void *sock, int msgmax, int hwm, int buf);
ERRT set_push_sock_opts(void *sock, int hwm, int buf);
ERRT req_rep_single(char const *addr, char const *msg, int n);
