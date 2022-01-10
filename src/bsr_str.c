/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_str.h>

ERRT bsr_str_split(char *str, int n, struct SplitMap *res) {
    res->n = 0;
    char *p1 = (char *)str;
    char *p2 = (char *)str + n;
    res->beg[res->n] = str;
    while (p1 < p2) {
        if (*p1 == ',') {
            res->end[res->n] = p1;
            res->n += 1;
            res->beg[res->n] = p1 + 1;
            if (res->n >= 7) {
                break;
            }
        };
        p1 += 1;
    };
    res->end[res->n] = p2;
    res->n += 1;
    return 0;
}

int64_t time_delta_mu(struct timespec t1, struct timespec t2) {
    return (int64_t)(t2.tv_sec * 1000000 + t2.tv_nsec / 1000) - (int64_t)(t1.tv_sec * 1000000 + t1.tv_nsec / 1000);
}
