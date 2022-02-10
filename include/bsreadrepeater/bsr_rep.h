#pragma once
#include <err.h>

struct bsrep {
    char *cmd_addr;
    char *startup_file;
};

ERRT cleanup_bsrep(struct bsrep *self);
ERRT bsrep_init(struct bsrep *self, char *cmd_addr, char *startup_file);
ERRT bsrep_run(struct bsrep *self);
