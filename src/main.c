#include <bsreadrepeater.h>
#include <zmq.h>

#define OKRET(ec)  \
    if ((ec) != 0) \
    {              \
        return ec; \
    }

int set_opts(void *sock)
{
    int ec;
    int linger = 800;
    ec = zmq_setsockopt(sock, ZMQ_LINGER, &linger, sizeof(int));
    OKRET(ec);
    return 0;
}

int main(int argc, char **argv)
{
    int ec;
    void *ctx = zmq_ctx_new();
    void *s1 = zmq_socket(ctx, ZMQ_PUSH);
    void *s2 = zmq_socket(ctx, ZMQ_PULL);
    ec = set_opts(s1);
    OKRET(ec);
    ec = set_opts(s2);
    OKRET(ec);
    ec = zmq_bind(s1, "tcp://127.0.0.1:9155");
    if (ec != 0)
    {
        return -1;
    }
    ec = zmq_connect(s2, "tcp://127.0.0.1:9155");
    if (ec != 0)
    {
        return -1;
    }

    ec = zmq_send(s1, "123456", 6, 0);
    if (ec != 6)
    {
        return -1;
    }

    uint8_t buf[16];
    ec = zmq_recv(s2, buf, 16, 0);
    if (ec != 6)
    {
        return -1;
    }
    fprintf(stderr, "hi there [%.6s]\n", buf);

    ec = zmq_close(s1);
    if (ec != 0)
    {
        return -1;
    }
    ec = zmq_close(s2);
    if (ec != 0)
    {
        return -1;
    }
    if (0)
    {
        ec = zmq_ctx_term(ctx);
        if (ec != 0)
        {
            return -1;
        }
    }
    ec = zmq_ctx_destroy(ctx);
    if (ec != 0)
    {
        return -1;
    }
    return 0;
}
