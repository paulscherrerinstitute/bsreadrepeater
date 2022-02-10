/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once

typedef int ERRT;

// Helpers to return on error.

#define NZRET(ec)                                                                                                      \
    if ((ec) != 0) {                                                                                                   \
        return ec;                                                                                                     \
    }

#define NULLRET(ptr)                                                                                                   \
    if ((ptr) == NULL) {                                                                                               \
        return -1;                                                                                                     \
    }

#define ZMQ_NEGONERET(ec)                                                                                              \
    if ((ec) == -1) {                                                                                                  \
        fprintf(stderr, "error: %d  %s\n", errno, zmq_strerror(errno));                                                \
        return -1;                                                                                                     \
    }

#define ZMQ_NULLRET(ptr)                                                                                               \
    if ((ptr) == NULL) {                                                                                               \
        fprintf(stderr, "error: %d  %s\n", errno, zmq_strerror(errno));                                                \
        return -1;                                                                                                     \
    }
