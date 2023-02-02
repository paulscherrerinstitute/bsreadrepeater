#pragma once
#include <bsr_cmdchn.h>
#include <bsr_dummy_source_s.h>
#include <bsr_poller.h>
#include <err.h>

ERRT dummy_source_init(struct bsr_dummy_source *self, struct bsr_poller *poller);
ERRT dummy_source_cleanup(struct bsr_dummy_source *self);
