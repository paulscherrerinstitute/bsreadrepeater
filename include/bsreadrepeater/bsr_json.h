/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#include <bsr_pod.h>
#include <err.h>
#include <glib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct channel_map;

struct bsread_main_header {
    int compr;
    uint64_t pulse;
    uint64_t timestamp;
};

ERRT json_parse_main_header(char const *str, int n, struct bsread_main_header *header, GString **log);
ERRT json_parse_data_header(char const *str, int n, struct bsread_data_header *header, uint64_t now,
                            struct channel_map *chnmap, struct bsread_main_header *mh, GString **log);
ERRT json_parse(char const *str, int n, GString **log);

struct channel_map *channel_map_new();
void channel_map_release(struct channel_map *self);
ERRT channel_map_str(struct channel_map *self, GString **out);

uint64_t now_us(void);

#ifdef __cplusplus
}
#endif
