#include <bsr_str.h>
#include <bsr_timed_events.h>
#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define TREE_KEY uint64_t
#define TREE_VAL struct timed_event

static gint ts_compare(gconstpointer a, gconstpointer b) {
    TREE_KEY x = *((TREE_KEY *)a);
    TREE_KEY y = *((TREE_KEY *)b);
    if (x < y) {
        return -1;
    } else if (x > y) {
        return +1;
    } else {
        return 0;
    }
}

ERRT bsr_timed_events_init(struct bsr_timed_events *self) {
    // TODO check for failed new?
    self->tree = g_tree_new(ts_compare);
    clock_gettime(CLOCK_MONOTONIC, &self->ts_init);
    return 0;
}

ERRT bsr_timed_events_add_input_enable(struct bsr_timed_events *self, char const *addr, uint64_t tsmu) {
    // TODO increase limit, fail if limit reached.
    if (g_tree_nnodes(self->tree) > 7) {
        return 0;
    }
    // TODO add cleanup handler in case something fails
    TREE_KEY *key = malloc(sizeof(TREE_KEY));
    if (key == NULL) {
        fprintf(stderr, "ERROR  malloc\n");
        return -1;
    }
    // TODO add cleanup handler in case something fails
    TREE_VAL *val = malloc(sizeof(TREE_VAL));
    if (val == NULL) {
        fprintf(stderr, "ERROR  malloc\n");
        return -1;
    }
    *key = tsmu;
    val->ty = ENABLE;
    strncpy(val->addr, addr, 78);
    g_tree_insert(self->tree, key, val);
    key = NULL;
    val = NULL;
    return 0;
}

static gboolean stop_on_first(gpointer key, gpointer value, gpointer ud) {
    *((gpointer *)ud) = key;
    return TRUE;
}

ERRT bsr_timed_events_most_recent_key(struct bsr_timed_events *self, uint64_t **retkey, struct timed_event **ret) {
    // TODO return only a result if time has been reached!
    struct timespec tsnow;
    clock_gettime(CLOCK_MONOTONIC, &tsnow);
    uint64_t tsnowmu = (uint64_t)time_delta_mu(self->ts_init, tsnow);
    *retkey = 0;
    *ret = NULL;
    GTree *tree = self->tree;
    if (g_tree_nnodes(tree) > 0) {
        // fprintf(stderr, "timed_events  present\n");
        TREE_KEY *key = NULL;
        g_tree_foreach(self->tree, stop_on_first, &key);
        // fprintf(stderr, "timed_events  search yields smallest ts_smallest %" PRIu64 "\n", *key);
        if (key != NULL) {
            if (*key <= tsnowmu) {
                TREE_VAL *ev = g_tree_lookup(tree, key);
                if (FALSE) {
                    fprintf(stderr, "timed_events  lookup yields %p\n", (void *)ev);
                    fprintf(stderr, "timed_events  value yields addr %.30s\n", ev->addr);
                }
                *retkey = key;
                *ret = ev;
            } else {
                // fprintf(stderr, "timed_events  earliest event not yet ready\n");
            }
        } else {
            fprintf(stderr, "ERROR  timed_events  timed events tree not empty but can not find first\n");
        }
    }
    return 0;
}
