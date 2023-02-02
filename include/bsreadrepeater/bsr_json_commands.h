#pragma once
#include <err.h>

#define SOURCE_ADDR_MAX (128)

struct command_add_source {
    char source[SOURCE_ADDR_MAX];
    int rcvhwm;
    int rcvbuf;
};

struct command_remove_source {
    int dummy;
};

struct command_add_output {
    char source[SOURCE_ADDR_MAX];
    char output[SOURCE_ADDR_MAX];
    int sndhwm;
    int sndbuf;
};

struct command_exit {
    int dummy;
};

enum bsr_json_command_kind {
    ADD_SOURCE,
    REMOVE_SOURCE,
    ADD_OUTPUT,
    EXIT,
    DUMMY_SOURCE_START,
};

union bsr_json_command_union {
    struct command_add_source add_source;
    struct command_remove_source remove_source;
    struct command_add_output add_output;
    struct command_exit exit;
};

struct bsr_json_command {
    enum bsr_json_command_kind kind;
    union bsr_json_command_union inner;
};

#ifdef __cplusplus
extern "C" {
#endif

ERRT parse_json_command(char const *str, int n, struct bsr_json_command *cmd);

#ifdef __cplusplus
}
#endif
