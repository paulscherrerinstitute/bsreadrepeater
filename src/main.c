/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#define ZMQ_BUILD_DRAFT_API
#include <bsr_chnhandler.h>
#include <bsr_cmdchn.h>
#include <bsr_ctx1.h>
#include <bsr_poller.h>
#include <bsr_rep.h>
#include <bsr_sockhelp.h>
#include <bsr_startupcmd.h>
#include <bsr_str.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>

int main_inner(int const argc, char const *const *argv) {
    int ec;
    int const cmd_addr_max = 64;
    char cmd_addr[cmd_addr_max];
    strncpy(cmd_addr, "tcp://127.0.0.1:4242", cmd_addr_max);
    if (argc < 2) {
        fprintf(stderr, "Usage: bsrep <COMMAND_SOCKET_ADDRESS> [STARTUP_COMMAND_FILE]\n\n");
        return 0;
    }
    if (argc >= 2) {
        strncpy(cmd_addr, argv[1], cmd_addr_max);
        cmd_addr[cmd_addr_max - 1] = 0;
    }
    fprintf(stderr, "Command socket address: %s\n", cmd_addr);

    char *startup_file = NULL;
    if (argc >= 3) {
        startup_file = malloc(strlen(argv[2]));
        strcpy(startup_file, argv[2]);
        fprintf(stderr, "Read startup commands from: %s\n", startup_file);
    }

    struct bsrep __attribute__((cleanup(cleanup_bsrep))) rep = {0};
    ec = bsrep_init(&rep, cmd_addr, startup_file);
    NZRET(ec);
    ec = bsrep_start(&rep);
    NZRET(ec);
    ec = bsrep_wait_for_done(&rep);
    NZRET(ec);
    return 0;
}

ERRT test_simple_start_and_shutdown() {
    int ec;
    int const cmd_addr_max = 64;
    char cmd_addr[cmd_addr_max];
    strncpy(cmd_addr, "tcp://127.0.0.1:4242", cmd_addr_max);
    char *startup_file = NULL;
    struct bsrep __attribute__((cleanup(cleanup_bsrep))) rep = {0};
    ec = bsrep_init(&rep, cmd_addr, startup_file);
    NZRET(ec);
    rep.do_polls_max = 2;
    ec = bsrep_start(&rep);
    NZRET(ec);
    ec = bsrep_wait_for_done(&rep);
    NZRET(ec);
    if (rep.polls_count != 2) {
        fprintf(stderr, "polls done: %lu\n", rep.polls_count);
        return 1;
    }
    return 0;
}

ERRT test_receive() {
    int ec;
    fprintf(stderr, "test_receive\n");
    struct ctx1 __attribute__((cleanup(cleanup_ctx1))) ctx1 = {0};
    ec = ctx1_init(&ctx1);
    NZRET(ec);
    ec = ctx1_start(&ctx1);
    NZRET(ec);
    struct bsrep __attribute__((cleanup(cleanup_bsrep))) rep = {0};
    rep.do_polls_min = 20;
    char const *cmd_addr = "tcp://127.0.0.1:4242";
    ec = bsrep_init(&rep, cmd_addr, NULL);
    NZRET(ec);
    ec = bsrep_start(&rep);
    NZRET(ec);
    ec = req_rep_single(cmd_addr, "blabla", 6);
    NZRET(ec);
    return 0;
}

int main(int const argc, char const *const *argv) {
    int ec;
    fprintf(stderr, "bsrep 0.3.0\n");
    {
        // For shared libs, make sure that we got dynamically linked against a compatible version.
        int maj, min, pat;
        zmq_version(&maj, &min, &pat);
        if (maj != 4 || min < 3) {
            fprintf(stderr, "ERROR require libzmq version 4.3 or compatible\n");
            return 1;
        };
        if (zmq_has("draft") != 1) {
            fprintf(stderr, "ERROR libzmq has no support for zmq_poller\n");
            return 1;
        };
    }
    int do_test = 0;
    if (argc >= 2) {
        if (strcmp("test", argv[1]) == 0) {
            do_test = 1;
        }
    }
    if (do_test) {
        ec = test_simple_start_and_shutdown();
        if (ec != 0) {
            fprintf(stderr, "ERROR test_simple_start_and_shutdown\n");
            return 1;
        }
        ec = test_receive();
        if (ec != 0) {
            fprintf(stderr, "ERROR test_receive\n");
            return 1;
        }
        return 0;
    } else {
        int x = main_inner(argc, argv);
        fprintf(stderr, "bsrep exit(%d)\n", x);
        return x;
    }
}
