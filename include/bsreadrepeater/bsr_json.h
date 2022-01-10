/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#include <err.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bsread_main_header {
    int compr;
};

struct bsread_data_header {
    int compr;
    GArray *channels;
};

ERRT json_parse_main_header(char const *str, int n, struct bsread_main_header *header, GString *log);
ERRT json_parse_data_header(char const *str, int n, struct bsread_data_header *header, GString *log);
ERRT json_parse(char const *str, int n, GString *log);

#ifdef __cplusplus
}
#endif
