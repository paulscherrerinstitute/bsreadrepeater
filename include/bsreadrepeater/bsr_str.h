/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#include <err.h>
#include <stdint.h>
#include <time.h>

struct SplitMap {
    char *beg[8];
    char *end[8];
    int n;
};

ERRT bsr_str_split(char *str, int n, struct SplitMap *res);
int64_t time_delta_mu(struct timespec t1, struct timespec t2);
