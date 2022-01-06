/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#pragma once
#define ZMQ_BUILD_DRAFT_API
#include <bsr_chnhandler.h>
#include <bsr_poller.h>
#include <err.h>
#include <glib.h>
#include <zmq.h>

struct HandlerList {
    GList *list;
};

struct bsr_cmdchn {
    void *socket;
    char addr[ADDR_CAP];
    void *user_data;
    int state;
    // not owned:
    struct HandlerList *handler_list;
    // not owned:
    struct bsr_statistics *stats;
};

struct CommandHandler {
    struct bsr_cmdchn cmdchn;
};

enum HandlerKind {
    CommandHandler = 1,
    SourceHandler = 2,
};

union HandlerUnion {
    struct CommandHandler cmdh;
    struct bsr_chnhandler src;
};

struct Handler {
    enum HandlerKind kind;
    union HandlerUnion handler;
};

void cleanup_struct_Handler(struct Handler *self);

enum CommandType {
    CmdNone = 0,
    CmdExit = 1,
    CmdAdd = 2,
    CmdRemove = 3,
};

struct CommandAdd {
    int port;
};

union CommandUnion {
    struct CommandAdd add;
};

struct ReceivedCommand {
    enum CommandType ty;
    union CommandUnion var;
};

ERRT handler_list_add(struct HandlerList *self, struct Handler *handler);
ERRT handler_list_remove(struct HandlerList *self, struct Handler *handler);
struct bsr_chnhandler *handler_list_find_by_input_addr(struct HandlerList *self, char const *addr);
struct Handler *handler_list_find_by_input_addr_2(struct HandlerList *self, char const *addr);

void cleanup_bsr_cmdchn(struct bsr_cmdchn *self);
int bsr_cmdchn_init(struct bsr_cmdchn *self, struct bsr_poller *poller, void *user_data, char *addr,
                    struct HandlerList *handler_list, struct bsr_statistics *stats);
int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, struct bsr_poller *poller, struct ReceivedCommand *cmd);
