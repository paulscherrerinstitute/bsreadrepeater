#define ZMQ_BUILD_DRAFT_API
#include <bsr_poller.h>
#include <err.h>
#include <zmq.h>

enum CommandType {
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

struct bsr_cmdchn {
    void *socket;
    int state;
};

void cleanup_bsr_cmdchn(struct bsr_cmdchn *self);
int bsr_cmdchn_init(struct bsr_cmdchn *self, struct bsr_poller *poller, void *user_data, int port);
int bsr_cmdchn_handle_event(struct bsr_cmdchn *self, struct bsr_poller *poller, struct ReceivedCommand *cmd);
