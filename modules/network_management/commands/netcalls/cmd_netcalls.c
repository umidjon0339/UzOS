/*
 * cmd_netcalls.c - `netcalls [group]`: call the network syscalls from
 * ring 3, check each answer.
 *
 * The socket syscalls are meant for user programs, so the honest test
 * is a user program. Each GROUP is one ring 3 program (a .s file in this
 * folder), run as user 1 (guest) through usertask. It makes its calls
 * with int 0x80 and leaves every answer in the usertask mailbox; this
 * command prints them next to what a correct kernel answers:
 *
 *   socket(UDP)                 3 ok    socket(UDP)                 4 ok
 *   bind(127.0.0.1:0)           0 ok    getsockname                 0 ok
 *   ...
 *   udp: 27 of 27 answers as expected
 *
 * Some answers are refusals on purpose: a guest may not open a raw
 * socket (-1 EPERM), bind port 53 (-13 EACCES) or rename the host.
 *
 *   netcalls          every group
 *   netcalls udp      one group: udp, tcp, local
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox
 *       (process_management/core/usertask), process_kill, strcmp, ui_*.
 */

#include "modules/network_management/commands/netcalls/cmd_netcalls.h"    // this command
#include "modules/process_management/core/usertask/usertask.h"            // usertask_start, usertask_wait, usertask_mailbox
#include "modules/process_management/core/process/process.h"              // process_kill
#include "lib/string/string.h"                                            // strcmp
#include "modules/ui/core/style/ui.h"                                     // ui_printf, ui_color

#define GUEST_UID 1
#define WAIT_MS   5000
#define DONE      0x516             // what each program writes to mailbox[0] at the end

typedef enum {
    EQ,                             // exactly 'value'
    FD,                             // a file descriptor (>= 3)
    AT_LEAST,                       // 'value' or more
} expect_t;

typedef struct {
    const char *call;
    expect_t    expect;
    int32_t     value;
} check_t;

// From the .s programs in this folder, one per group
extern uint8_t udp_program[], udp_program_end[];
extern uint8_t tcp_program[], tcp_program_end[];
extern uint8_t local_program[], local_program_end[];

static const check_t udp_checks[] = {
    [1]  = { "socket(UDP)",           FD, 0 },
    [2]  = { "socket(UDP)",           FD, 0 },
    [3]  = { "bind(127.0.0.1:0)",     EQ, 0 },
    [4]  = { "getsockname",           EQ, 0 },
    [5]  = { "  the port chosen",     AT_LEAST, 32768 },
    [6]  = { "sendto(\"hello\")",     EQ, 5 },
    [7]  = { "recvfrom",              EQ, 5 },
    [8]  = { "  the same bytes",      EQ, 1 },
    [9]  = { "getpeername (unconn.)", EQ, -107 },
    [10] = { "connect",               EQ, 0 },
    [11] = { "write() on a socket",   EQ, 2 },
    [12] = { "read() on a socket",    EQ, 2 },
    [13] = { "setsockopt(RCVBUF)",    EQ, 0 },
    [14] = { "getsockopt(TYPE)",      EQ, 0 },
    [15] = { "  SOCK_DGRAM",          EQ, 2 },
    [16] = { "setsockopt(len 1)",     EQ, -22 },
    [17] = { "socket(RAW) as guest",  EQ, -1 },
    [18] = { "bind(port 53) as guest", EQ, -13 },
    [19] = { "sethostname as guest",  EQ, -1 },
    [20] = { "socketcall(SOCKET)",    FD, 0 },
    [21] = { "close",                 EQ, 0 },
    [22] = { "uname",                 EQ, 0 },
    [23] = { "  sysname \"UzOS\"",    EQ, 0x534F7A55 },
    [24] = { "gethostname",           EQ, 0 },
    [25] = { "gethostname(2 bytes)",  EQ, -22 },
    [26] = { "close",                 EQ, 0 },
    [27] = { "close",                 EQ, 0 },
};

static const check_t tcp_checks[] = {
    [1]  = { "socket(TCP)",           FD, 0 },
    [2]  = { "bind(127.0.0.1:7802)",  EQ, 0 },
    [3]  = { "listen",                EQ, 0 },
    [4]  = { "socket(TCP)",           FD, 0 },
    [5]  = { "connect",               EQ, 0 },
    [6]  = { "accept",                FD, 0 },
    [7]  = { "  peer 127.0.0.1",      EQ, 0x0100007F },
    [8]  = { "getpeername",           EQ, 0 },
    [9]  = { "send 6 bytes",          EQ, 6 },
    [10] = { "recv",                  EQ, 6 },
    [11] = { "sendmsg(3 + 4)",        EQ, 7 },
    [12] = { "recvmsg(4 + 10)",       EQ, 7 },
    [13] = { "  the same bytes",      EQ, 1 },
    [14] = { "setsockopt(NODELAY)",   EQ, 0 },
    [15] = { "shutdown(WR)",          EQ, 0 },
    [16] = { "recv: end of file",     EQ, 0 },
    [17] = { "accept4 (nonblock)",    EQ, -11 },
    [18] = { "close",                 EQ, 0 },
    [19] = { "close",                 EQ, 0 },
    [20] = { "close",                 EQ, 0 },
};

static const check_t local_checks[] = {
    [1]  = { "socketpair(UNIX)",      EQ, 0 },
    [2]  = { "write 5",               EQ, 5 },
    [3]  = { "read",                  EQ, 5 },
    [4]  = { "SO_PEERCRED: uid",      EQ, 1 },
    [5]  = { "poll (nothing yet)",    EQ, 0 },
    [6]  = { "poll (data)",           EQ, 1 },
    [7]  = { "  revents POLLIN",      EQ, 1 },
    [8]  = { "select",                EQ, 1 },
    [9]  = { "ioctl(FIONREAD)",       EQ, 5 },
    [10] = { "epoll_create1",         FD, 0 },
    [11] = { "epoll_ctl(ADD)",        EQ, 0 },
    [12] = { "epoll_wait",            EQ, 1 },
    [13] = { "  our data back",       EQ, 0x1234 },
    [14] = { "read",                  EQ, 5 },
    [15] = { "ioctl(FIONBIO)",        EQ, 0 },
    [16] = { "read (empty, nonblock)", EQ, -11 },
    [17] = { "bind(\"/tmp/netcalls\")", EQ, 0 },
    [18] = { "listen",                EQ, 0 },
    [19] = { "connect",               EQ, 0 },
    [20] = { "accept",                FD, 0 },
    [21] = { "close (all)",           EQ, 0 },
};

static const struct {
    const char    *name;
    const uint8_t *start, *end;
    const check_t *checks;
    int            count;           // slots 1..count
} groups[] = {
    { "udp", udp_program, udp_program_end, udp_checks,
      sizeof(udp_checks) / sizeof(udp_checks[0]) - 1 },
    { "tcp", tcp_program, tcp_program_end, tcp_checks,
      sizeof(tcp_checks) / sizeof(tcp_checks[0]) - 1 },
    { "local", local_program, local_program_end, local_checks,
      sizeof(local_checks) / sizeof(local_checks[0]) - 1 },
};
#define GROUP_COUNT (int)(sizeof(groups) / sizeof(groups[0]))

static int as_expected(const check_t *c, int32_t got) {
    switch (c->expect) {
        case FD:       return got >= 3;
        case AT_LEAST: return got >= c->value;
        default:       return got == c->value;
    }
}

static int total_good, total_count;    // over every group run by one command

// Run one group's program and print its answers. Returns 1 if all were right.
static int run_group(int g) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start(groups[g].name, groups[g].start, groups[g].end, GUEST_UID);
    if (pid < 0) {
        ui_error("netcalls", "could not start the ring 3 program");
        return 0;
    }
    if (!usertask_wait(pid, WAIT_MS))
        process_kill(pid);                  // stuck in a call: do not leave it behind
    if (box[0] != DONE) {
        ui_error("netcalls", "the program did not finish");
        return 0;
    }

    int good = 0, count = groups[g].count;
    ui_printf("\n%s: ring 3 program PID %d, user guest\n", groups[g].name, pid);
    for (int slot = 1; slot <= count; slot++) {
        const check_t *c = &groups[g].checks[slot];
        int32_t got = (int32_t)box[slot];
        int ok = as_expected(c, got);
        good += ok;
        ui_printf("%-23s%6d ", c->call, got);
        ui_color(ok ? UI_GOOD : UI_BAD);
        vga_print_str(ok ? "ok" : "NO");
        ui_reset();
        vga_print_str(slot % 2 ? "    " : "\n");      // two columns
    }
    vga_print_str(count % 2 ? "\n" : "");
    ui_start(good == count ? UI_OK : UI_ERROR, "netcalls");
    ui_printf("%s: %d of %d answers as expected", groups[g].name, good, count);
    ui_end();
    total_good += good;
    total_count += count;
    return good == count;
}

void cmd_netcalls(const char *args) {
    int ran = 0;
    total_good = total_count = 0;
    for (int g = 0; g < GROUP_COUNT; g++) {
        if (args[0] && strcmp(args, groups[g].name) != 0)
            continue;
        run_group(g);
        ran++;
    }
    if (!ran) {
        ui_usage("netcalls [udp | tcp | local]");
        return;
    }
    if (ran > 1) {
        ui_start(total_good == total_count ? UI_OK : UI_ERROR, "netcalls");
        ui_printf("all groups: %d of %d answers as expected", total_good, total_count);
        ui_end();
    }
}
