#pragma once
#include <err.h>

ERRT set_basic_sock_opts(void *sock);
ERRT set_linger(void *sock, int linger);
ERRT set_rcvhwm(void *sock, int rcvhwm);
ERRT set_sndhwm(void *sock, int sndhwm);
ERRT set_rcvbuf(void *sock, int rcvbuf);
ERRT set_sndbuf(void *sock, int sndbuf);
