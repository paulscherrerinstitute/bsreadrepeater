#pragma once

typedef int ERRT;

// Helper to return on error.
#define NZRET(ec)                                                              \
    if ((ec) != 0) {                                                           \
        return ec;                                                             \
    }

// Helper to return on error.
#define NULLRET(ptr)                                                           \
    if ((ptr) == NULL) {                                                       \
        return -1;                                                             \
    }
