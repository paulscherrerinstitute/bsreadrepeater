#include <bsr_dummy_source.h>
#include <zmq.h>

void timer_cb_1(int id, void *arg) {
    fprintf(stderr, "timer called\n");
    struct bsr_dummy_source *self = (struct bsr_dummy_source *)arg;
    zmq_send(self->sock, "timed", 5, ZMQ_DONTWAIT);
}

ERRT dummy_source_init(struct bsr_dummy_source *self, struct bsr_poller *poller) {
    int ec;
    self->sock = zmq_socket(poller->ctx, ZMQ_PUSH);
    NULLRET(self->sock);
    int opt = 200;
    ec = zmq_setsockopt(self->sock, ZMQ_LINGER, &opt, sizeof(opt));
    EMSG(ec, "zmq_setsockopt");
    ec = zmq_bind(self->sock, "tcp://127.0.0.1:3986");
    EMSG(ec, "zmq_bind");
    self->timer_id = zmq_timers_add(poller->timer_poller, 1000, timer_cb_1, self);
    EMSG(ec, "zmq_timers_add");
    self->timer_in_use = 1;
    return 0;
}

ERRT dummy_source_cleanup(struct bsr_dummy_source *self) {
    int ec;
    if (self->sock != NULL) {
        ec = zmq_close(self->sock);
        self->sock = NULL;
        EMSG(ec, "zmq_close");
    }
    if (self->timer_in_use != 0) {
        zmq_timers_cancel(self->poller->timer_poller, self->timer_id);
        self->timer_id = 0;
        self->timer_in_use = 0;
    }
    return 0;
}
