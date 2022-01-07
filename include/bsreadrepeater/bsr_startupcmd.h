/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <glib.h>
#include <zmq.h>

struct bsr_startupcmd {
    // borrowed:
    struct bsr_poller *poller;
    void *user_data;
    int fd;
    void *sock_out;
    char cmdaddr[80];
    char buf[256];
    char *wp;
    char *rp;
    char *ep;
    char cmd[128];
    int state;
};

ERRT cleanup_struct_bsr_startupcmd(struct bsr_startupcmd *self);
ERRT bsr_startupcmd_init(struct bsr_startupcmd *self, struct bsr_poller *poller, void *user_data, int fd,
                         char *cmdaddr);
ERRT bsr_startupcmd_handle_event(struct bsr_startupcmd *self);
