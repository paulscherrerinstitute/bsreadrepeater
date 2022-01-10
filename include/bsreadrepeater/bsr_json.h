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

ERRT json_parse(char *s, int n, GString *log);

#ifdef __cplusplus
}
#endif
