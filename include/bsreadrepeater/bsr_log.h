#pragma once
#include <stdio.h>
#include <stdlib.h>

#define INFO(fmt, ...)                                                                                                 \
    { fprintf(stderr, "INFO " fmt "\n", ##__VA_ARGS__); }

#define DEBUG(fmt, ...)                                                                                                \
    { fprintf(stderr, "DEBUG " fmt "\n", ##__VA_ARGS__); }

#define TRACE(fmt, ...)                                                                                                \
    { fprintf(stderr, "TRACE " fmt "\n", ##__VA_ARGS__); }
