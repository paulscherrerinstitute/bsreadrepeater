/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <hex.h>
#include <stdio.h>

void to_hex(char *buf, uint8_t *in, int n) {
    char *p1 = buf;
    for (int i = 0; i < n; i += 1) {
        snprintf(p1, 3, "%02x", in[i]);
        p1 += 2;
    }
    *p1 = 0;
}
