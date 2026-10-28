/*
 * cmd_cpucalls.c - `cpucalls`: call every CPU syscall from ring 3, check each answer.
 *
 * The syscalls of cpu_management are meant for user programs, so the
 * honest test is a user program: cpucalls.s runs in ring 3 as user 1
 * (guest), goes through all of them with int 0x80 (the [syscall] trace
 * shows each one), and leaves every answer in the usertask mailbox.
 * This command then prints them next to what a correct kernel answers:
 *
 *   getpriority(me)        20 ok    setpriority(me, -5)    -1 ok
 *   ...
 *   63 of 63 answers as expected
 *
 * Some answers are refusals on purpose (-1): a guest may not lower its
 * nice, use SCHED_FIFO, raise a hard limit, or ask for a CPU or NUMA
 * node that does not exist. The last part makes two real threads with
 * clone() and clone3(), which report their own ids and end with exit();
 * exit_group() then ends the program.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox
 *       (process_management/core/usertask), ui_* (ui).
 */

#include "modules/cpu_management/commands/cpucalls/cmd_cpucalls.h"     // this command
#include "modules/process_management/core/usertask/usertask.h"         // usertask_*
#include "modules/ui/core/style/ui.h"                                  // ui_*

#define GUEST_UID 1
#define WAIT_MS   3000
#define DONE      0xCA11        // what cpucalls.s writes to mailbox[0] at the end
#define SLOTS     63

// From cpucalls.s
extern uint8_t cpucalls_program[], cpucalls_program_end[];

typedef enum {
    EQ,         // exactly 'value'
    PID,        // the program's own pid
    FD,         // a file descriptor (>= 3)
    AT_LEAST,   // 'value' or more
    SAME_AS,    // the same as slot 'value'
    NEW_TASK,   // a new pid (above the program's)
} expect_t;

static const struct {
    const char *call;
    expect_t    expect;
    int32_t     value;
} checks[SLOTS + 1] = {
    [1]  = { "getpid()",               PID,      0 },
    [2]  = { "gettid()",               PID,      0 },
    [3]  = { "sched_yield()",          EQ,       0 },
    [4]  = { "getpriority(me)",        EQ,       20 },      // 20 - nice 0
    [5]  = { "setpriority(me, 5)",     EQ,       0 },
    [6]  = { "getpriority(me)",        EQ,       15 },      // 20 - 5
    [7]  = { "setpriority(me, -5)",    EQ,       -1 },      // users only nicer
    [8]  = { "sched_getscheduler",     EQ,       0 },       // SCHED_OTHER
    [9]  = { "setscheduler(BATCH)",    EQ,       0 },
    [10] = { "sched_getscheduler",     EQ,       3 },       // SCHED_BATCH
    [11] = { "setscheduler(FIFO 10)",  EQ,       -1 },      // no RLIMIT_RTPRIO
    [12] = { "sched_getparam(me)",     EQ,       0 },
    [13] = { "  param.priority",       EQ,       0 },
    [14] = { "sched_setparam(5)",      EQ,       -1 },      // BATCH takes 0 only
    [15] = { "get_priority_min(FIFO)", EQ,       1 },
    [16] = { "get_priority_max(FIFO)", EQ,       99 },
    [17] = { "sched_rr_get_interval",  EQ,       0 },
    [18] = { "  slice in ms",          EQ,       50 },
    [19] = { "sched_getaffinity",      EQ,       4 },       // bytes
    [20] = { "  mask",                 EQ,       1 },       // CPU 0
    [21] = { "setaffinity(CPU 1)",     EQ,       -1 },      // no CPU 1
    [22] = { "setaffinity(CPU 0)",     EQ,       0 },
    [23] = { "sched_getattr(me)",      EQ,       0 },
    [24] = { "  attr.policy",          EQ,       3 },
    [25] = { "  attr.nice",            EQ,       5 },
    [26] = { "setattr(OTHER, nice 7)", EQ,       0 },
    [27] = { "getpriority(me)",        EQ,       13 },      // 20 - 7
    [28] = { "getrlimit(NOFILE)",      EQ,       0 },
    [29] = { "  rlim.cur",             EQ,       16 },
    [30] = { "setrlimit(NOFILE 8/8)",  EQ,       0 },       // lowering: fine
    [31] = { "setrlimit(NOFILE 16)",   EQ,       -1 },      // raising hard: root
    [32] = { "prlimit64(me, NOFILE)",  EQ,       0 },
    [33] = { "  old.cur",              EQ,       8 },
    [34] = { "getcpu()",               EQ,       0 },
    [35] = { "  cpu",                  EQ,       0 },
    [36] = { "  node",                 EQ,       0 },
    [37] = { "perf_event_open(cs)",    FD,       0 },
    [38] = { "read(fd, 8)",            EQ,       8 },
    [39] = { "  switches (2 sleeps)",  AT_LEAST, 2 },
    [40] = { "close(fd)",              EQ,       0 },
    [41] = { "perf(instructions)",     EQ,       -1 },      // needs a PMU
    [42] = { "set_mempolicy(BIND 0)",  EQ,       0 },
    [43] = { "get_mempolicy()",        EQ,       0 },
    [44] = { "  mode",                 EQ,       2 },       // MPOL_BIND
    [45] = { "set_mempolicy(BIND 1)",  EQ,       -1 },      // no node 1
    [46] = { "mbind(stack, PREF 0)",   EQ,       0 },
    [47] = { "  policy at stack",      EQ,       1 },       // MPOL_PREFERRED
    [48] = { "get_mempolicy(NODE)",    EQ,       0 },
    [49] = { "  node of stack",        EQ,       0 },
    [50] = { "move_pages(2 pages)",    EQ,       0 },
    [51] = { "  status[0] code",       EQ,       0 },       // node 0
    [52] = { "  status[1] unmapped",   EQ,       -2 },      // ENOENT
    [53] = { "migrate_pages(0->0)",    EQ,       0 },
    [54] = { "membarrier(QUERY)",      EQ,       31 },
    [55] = { "membarrier(PRIVATE)",    EQ,       -1 },      // not registered yet
    [56] = { "membarrier(REGISTER)",   EQ,       0 },
    [57] = { "membarrier(PRIVATE)",    EQ,       0 },
    [58] = { "clone(VM|THREAD)",       NEW_TASK, 0 },
    [59] = { "  *parent_tid",          SAME_AS,  58 },
    [60] = { "  thread: gettid()",     SAME_AS,  58 },
    [61] = { "  thread: getpid()",     PID,      0 },       // the group's id
    [62] = { "clone(no CLONE_VM)",     EQ,       -1 },      // needs fork()
    [63] = { "clone3(VM|THREAD)",      NEW_TASK, 0 },
};

static int as_expected(int slot, int32_t got, int32_t pid, volatile uint32_t *box) {
    int32_t v = checks[slot].value;
    switch (checks[slot].expect) {
        case PID:      return got == pid;
        case FD:       return got >= 3;
        case AT_LEAST: return got >= v;
        case SAME_AS:  return got == (int32_t)box[v];
        case NEW_TASK: return got > pid;
        default:       return got == v;
    }
}

void cmd_cpucalls(const char *args) {
    (void)args;
    volatile uint32_t *box = usertask_mailbox();

    int pid = usertask_start("cpucalls", cpucalls_program, cpucalls_program_end, GUEST_UID);
    if (pid < 0) {
        ui_error("cpucalls", "could not start the ring 3 program");
        return;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != DONE) {
        ui_error("cpucalls", "the program did not finish");
        return;
    }

    int good = 0;
    ui_printf("\nring 3 program PID %d, user guest:\n", pid);
    for (int slot = 1; slot <= SLOTS; slot++) {
        int32_t got = (int32_t)box[slot];
        int ok = as_expected(slot, got, pid, box);
        good += ok;

        ui_printf("%-23s%6d ", checks[slot].call, got);
        ui_color(ok ? UI_GOOD : UI_BAD);
        vga_print_str(ok ? "ok" : "NO");
        ui_reset();
        vga_print_str(slot % 2 ? "    " : "\n");      // two columns
    }
    vga_print_str("\n");
    ui_start(good == SLOTS ? UI_OK : UI_ERROR, "cpucalls");
    ui_printf("%d of %d answers as expected", good, SLOTS);
    ui_end();
}
