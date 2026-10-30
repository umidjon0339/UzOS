/*
 * cmd_proccalls.c - `proccalls [group]`: call the process syscalls from
 * ring 3, check each answer.
 *
 * The syscalls of process_management are meant for user programs, so
 * the honest test is a user program. Each GROUP is one ring 3 program
 * (a .s file in this folder), run as user 1 (guest) through usertask.
 * It goes through its syscalls with int 0x80 (the [syscall] trace shows
 * each one) and leaves every answer in the usertask mailbox. This
 * command prints them next to what a correct kernel answers:
 *
 *   sigaction(USR1, h1)        0 ok    old handler            0 ok
 *   ...
 *   signals: 49 of 49 answers as expected
 *
 * Some answers are refusals on purpose (-1): catching SIGKILL, a
 * handler inside the kernel, a forged rt_sigreturn.
 *
 *   proccalls            every group
 *   proccalls signals    one group: signals, prctl, wait, session, timers,
 *                        sandbox, debug, exec
 *
 * The "exec" group needs a program on disk: showargs.s is a complete ELF
 * file written byte by byte, which this command saves as
 * /home/guest/showargs first.
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox
 *       (process_management/core/usertask), sys_open / sys_write /
 *       sys_close (io_management), strcmp (lib/string), ui_* (ui).
 */

#include "modules/process_management/commands/proccalls/cmd_proccalls.h"  // this file
#include "modules/process_management/core/usertask/usertask.h"            // usertask_start, usertask_wait, usertask_mailbox
#include "modules/io_management/syscalls/open/sys_open.h"                 // sys_open, O_WRITE
#include "modules/io_management/syscalls/write/sys_write.h"               // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"               // sys_close
#include "lib/string/string.h"                                            // strcmp
#include "modules/ui/core/style/ui.h"                                     // ui_printf, ui_color, ui_error

#define GUEST_UID 1
#define WAIT_MS   6000          // the signals group waits for two alarms
#define DONE      0x516         // what each program writes to mailbox[0] at the end
#define SHOWARGS_PATH "/home/guest/showargs"

typedef enum {
    EQ,         // exactly 'value'
    PID,        // the program's own pid
    FD,         // a file descriptor (>= 3)
    AT_LEAST,   // 'value' or more
    SAME_AS,    // the same as slot 'value'
    NEW_TASK,   // a new pid (above the program's)
} expect_t;

typedef struct {
    const char *call;
    expect_t    expect;
    int32_t     value;
} check_t;

// From the .s programs in this folder, one per group
extern uint8_t signals_program[], signals_program_end[];
extern uint8_t prctl_program[], prctl_program_end[];
extern uint8_t wait_program[], wait_program_end[];
extern uint8_t session_program[], session_program_end[];
extern uint8_t timers_program[], timers_program_end[];
extern uint8_t sandbox_program[], sandbox_program_end[];
extern uint8_t debug_program[], debug_program_end[];
extern uint8_t exec_program[], exec_program_end[];
extern uint8_t showargs_elf[], showargs_elf_end[];   // a whole ELF file (showargs.s)

static const check_t signal_checks[] = {
    [1]  = { "sigaction(USR1, h1)",    EQ,  0 },
    [2]  = { "  old handler (DFL)",    EQ,  0 },
    [3]  = { "kill(me, USR1)",         EQ,  0 },
    [4]  = { "  h1 ran",               EQ,  1 },
    [5]  = { "  h1 argument",          EQ,  10 },
    [6]  = { "sigprocmask(BLOCK)",     EQ,  0 },
    [7]  = { "  old mask",             EQ,  0 },
    [8]  = { "kill(me, USR1)",         EQ,  0 },
    [9]  = { "  h1 ran (blocked)",     EQ,  1 },
    [10] = { "rt_sigpending",          EQ,  0 },
    [11] = { "  pending: USR1",        EQ,  1 << 10 },
    [12] = { "sigprocmask(UNBLOCK)",   EQ,  0 },
    [13] = { "  h1 ran (unblocked)",   EQ,  2 },
    [14] = { "sigaction(KILL, h1)",    EQ,  -1 },     // handler_bounds
    [15] = { "sigaction(h = kernel)",  EQ,  -1 },     // handler_bounds
    [16] = { "sigaction(USR2, IGN)",   EQ,  0 },
    [17] = { "kill(me, USR2)",         EQ,  0 },
    [18] = { "  pending (ignored)",    EQ,  0 },
    [19] = { "sigaltstack(1 KiB)",     EQ,  0 },
    [20] = { "  old: SS_DISABLE",      EQ,  2 },
    [21] = { "sigaction(h2, SIGINFO)", EQ,  0 },
    [22] = { "kill(me, USR1)",         EQ,  0 },
    [23] = { "  h2 on the altstack",   EQ,  1 },
    [24] = { "  info.signo",           EQ,  10 },
    [25] = { "  info.code SI_USER",    EQ,  0 },
    [26] = { "  info.pid",             PID, 0 },
    [27] = { "sigqueueinfo(1234)",     EQ,  0 },
    [28] = { "  info.value",           EQ,  1234 },
    [29] = { "  info.code SI_QUEUE",   EQ,  -1 },
    [30] = { "tgsigqueueinfo(77)",     EQ,  0 },
    [31] = { "  info.value",           EQ,  77 },
    [32] = { "  h2 ran",               EQ,  3 },
    [33] = { "sigaction(ALRM, h3)",    EQ,  0 },
    [34] = { "alarm(2)",               EQ,  0 },
    [35] = { "alarm(1): 2 s left",     EQ,  2 },
    [36] = { "pause()",                EQ,  -1 },     // EINTR, after the handler
    [37] = { "  SIGALRM handled",      EQ,  1 },
    [38] = { "sigaction(USR2, DFL)",   EQ,  0 },
    [39] = { "sigprocmask(BLOCK 2)",   EQ,  0 },
    [40] = { "kill(me, USR2)",         EQ,  0 },
    [41] = { "sigtimedwait(USR2)",     EQ,  12 },
    [42] = { "  info.signo",           EQ,  12 },
    [43] = { "sigtimedwait(50 ms)",    EQ,  -1 },     // time is up
    [44] = { "alarm(1)",               EQ,  0 },
    [45] = { "sigsuspend(none)",       EQ,  -1 },
    [46] = { "  SIGALRM handled",      EQ,  2 },
    [47] = { "sigprocmask(query)",     EQ,  0 },
    [48] = { "  mask is back: USR2",   EQ,  1 << 12 },
    [49] = { "forged sigreturn lived", EQ,  0 },      // sigframe_check ends it
};

static const check_t prctl_checks[] = {
    [1]  = { "SET_NAME(\"pc_ok\")",     EQ, 0 },
    [2]  = { "GET_NAME",               EQ, 0 },
    [3]  = { "  first 4 bytes",        EQ, 0x6F5F6370 },  // "pc_o"
    [4]  = { "SET_NAME(kernel_main)",  EQ, -1 },          // name_guard
    [5]  = { "SET_PDEATHSIG(TERM)",    EQ, 0 },
    [6]  = { "GET_PDEATHSIG",          EQ, 0 },
    [7]  = { "  signal",               EQ, 15 },
    [8]  = { "SET_PDEATHSIG(99)",      EQ, -1 },
    [9]  = { "GET_DUMPABLE",           EQ, 1 },
    [10] = { "SET_DUMPABLE(0)",        EQ, 0 },
    [11] = { "GET_DUMPABLE",           EQ, 0 },
    [12] = { "SET_DUMPABLE(1)",        EQ, 0 },
    [13] = { "SET_CHILD_SUBREAPER",    EQ, 0 },
    [14] = { "GET_CHILD_SUBREAPER",    EQ, 0 },
    [15] = { "  value",                EQ, 1 },
    [16] = { "GET_NO_NEW_PRIVS",       EQ, 0 },
    [17] = { "SET_NO_NEW_PRIVS(2)",    EQ, -1 },
    [18] = { "SET_NO_NEW_PRIVS(1)",    EQ, 0 },
    [19] = { "GET_NO_NEW_PRIVS",       EQ, 1 },
    [20] = { "prctl(12345)",           EQ, -1 },
};

static const check_t wait_checks[] = {
    [1]  = { "clone(child: exit 7)",   NEW_TASK, 0 },
    [2]  = { "wait4(it, &st, &ru)",    SAME_AS,  1 },
    [3]  = { "  status: 7 << 8",       EQ,       7 << 8 },
    [4]  = { "  rusage.maxrss KiB",    EQ,       4 },
    [5]  = { "wait4(-1, WNOHANG)",     EQ,       -1 },      // no children left
    [6]  = { "clone(child: SIGTERM)",  NEW_TASK, 0 },
    [7]  = { "wait4(-1, &st)",         SAME_AS,  6 },
    [8]  = { "  status: signal 15",    EQ,       15 },
    [9]  = { "clone(child: pause)",    NEW_TASK, 0 },
    [10] = { "wait4(it, WNOHANG)",     EQ,       0 },       // still running
    [11] = { "pidfd_open(it)",         FD,       0 },
    [12] = { "pidfd_send_signal(KILL)", EQ,      0 },
    [13] = { "waitid(P_PIDFD)",        SAME_AS,  9 },
    [14] = { "  info.code CLD_KILLED", EQ,       2 },
    [15] = { "  info.status SIGKILL",  EQ,       9 },
    [16] = { "pidfd_send_signal(gone)", EQ,      -1 },      // pidfd_handle
    [17] = { "close(pidfd)",           EQ,       0 },
    [18] = { "wait4(1) not mine",      EQ,       -1 },      // child_only
    [19] = { "pidfd_open(1) system",   EQ,       -1 },      // pidfd_handle
    [20] = { "getrusage(SELF)",        EQ,       0 },
    [21] = { "  nvcsw",                AT_LEAST, 1 },
    [22] = { "getrusage(CHILDREN)",    EQ,       0 },
    [23] = { "times() ticks",          AT_LEAST, 1 },
    [24] = { "open(pidfd.txt)",        FD,       0 },
    [25] = { "write(\"hi\")",          EQ,       2 },
    [26] = { "pidfd_open(me)",         FD,       0 },
    [27] = { "pidfd_getfd(me, file)",  FD,       0 },
    [28] = { "seek(copy, 0)",          EQ,       0 },
    [29] = { "read(copy, 2)",          EQ,       2 },
    [30] = { "  bytes \"hi\"",         EQ,       0x6968 },
    [31] = { "pidfd_getfd(fd 13)",     EQ,       -1 },      // not open
    [32] = { "clone(child: exit 0)",   NEW_TASK, 0 },
    [33] = { "waitpid(it)",            SAME_AS,  32 },
    [34] = { "wait() none left",       EQ,       -1 },
};

static const check_t session_checks[] = {
    [1]  = { "getppid()",              AT_LEAST, 1 },
    [2]  = { "getpgrp()",              AT_LEAST, 1 },
    [3]  = { "getsid(0)",              AT_LEAST, 1 },
    [4]  = { "setpgid(me, group 1)",   EQ,       -1 },      // session_check
    [5]  = { "setpgid(me, new)",       EQ,       0 },
    [6]  = { "getpgrp() = my pid",     PID,      0 },
    [7]  = { "setsid() as leader",     EQ,       -1 },      // session_check
    [8]  = { "clone(child: pause)",    NEW_TASK, 0 },
    [9]  = { "setpgid(child, new)",    EQ,       0 },
    [10] = { "getpgid(child)",         SAME_AS,  8 },
    [11] = { "kill(-group, 0)",        EQ,       0 },
    [12] = { "kill(-group, SIGKILL)",  EQ,       0 },
    [13] = { "wait4(child)",           SAME_AS,  8 },
    [14] = { "  status: SIGKILL",      EQ,       9 },
    [15] = { "kill(0, 0) my group",    EQ,       0 },
    [16] = { "kill(-99999, 0)",        EQ,       -1 },
};

static const check_t timer_checks[] = {
    [1]  = { "nanosleep(50 ms)",       EQ,       0 },
    [2]  = { "  ticks it took",        AT_LEAST, 5 },
    [3]  = { "nanosleep(nsec = 1 s)",  EQ,       -1 },      // sleep_bounds
    [4]  = { "nanosleep(-1 s)",        EQ,       -1 },      // sleep_bounds
    [5]  = { "clock_nanosleep(20 ms)", EQ,       0 },
    [6]  = { "clock_nanosleep(past)",  EQ,       0 },
    [7]  = { "clock_nanosleep(99)",    EQ,       22 },      // EINVAL
    [8]  = { "sigaction(ALRM)",        EQ,       0 },
    [9]  = { "setitimer(REAL 30 ms)",  EQ,       0 },
    [10] = { "pause()",                EQ,       -1 },
    [11] = { "  SIGALRMs",             EQ,       1 },
    [12] = { "setitimer(every 20ms)",  EQ,       0 },
    [13] = { "  after 3 pauses",       EQ,       4 },
    [14] = { "getitimer(REAL)",        EQ,       0 },
    [15] = { "  interval usec",        EQ,       20000 },
    [16] = { "setitimer(off)",         EQ,       0 },
    [17] = { "  old interval usec",    EQ,       20000 },
    [18] = { "sigaction(VTALRM)",      EQ,       0 },
    [19] = { "setitimer(VIRTUAL)",     EQ,       0 },
    [20] = { "  spun: SIGVTALRM",      EQ,       1 },
    [21] = { "sigaction(PROF)",        EQ,       0 },
    [22] = { "setitimer(PROF)",        EQ,       0 },
    [23] = { "  spun: SIGPROF",        EQ,       1 },
    [24] = { "setitimer(5)",           EQ,       -1 },
    [25] = { "setitimer(-1 s)",        EQ,       -1 },      // sleep_bounds
    [26] = { "setitimer(REAL 30 ms)",  EQ,       0 },
    [27] = { "nanosleep(1 s) cut",     EQ,       -1 },      // EINTR
    [28] = { "  rem.nsec",             AT_LEAST, 900000000 },
    [29] = { "  SIGALRMs",             EQ,       5 },
    [30] = { "alarm(1)",               EQ,       0 },
    [31] = { "  getitimer: sec left",  AT_LEAST, 0 },
    [32] = { "alarm(0): 1 s left",     EQ,       1 },
};

static const check_t sandbox_checks[] = {
    [1]  = { "unshare(0)",             EQ,       0 },
    [2]  = { "unshare(CLONE_FILES)",   EQ,       0 },       // already private
    [3]  = { "unshare(CLONE_NEWPID)",  EQ,       -1 },      // no namespaces
    [4]  = { "setns(3, 0)",            EQ,       -1 },
    [5]  = { "seccomp(ACTION_AVAIL)",  EQ,       0 },
    [6]  = { "seccomp(FILTER) no NNP", EQ,       -1 },      // no_new_privs
    [7]  = { "prctl(NO_NEW_PRIVS)",    EQ,       0 },
    [8]  = { "seccomp(FILTER)",        EQ,       0 },
    [9]  = { "getpid() allowed",       PID,      0 },
    [10] = { "open() filtered",        EQ,       -1 },      // seccomp_filter
    [11] = { "kill() filtered",        EQ,       -1 },      // seccomp_filter
    [12] = { "clone(child)",           NEW_TASK, 0 },
    [13] = { "  child: no_new_privs",  EQ,       1 },
    [14] = { "  child: open()",        EQ,       -1 },      // inherited
    [15] = { "wait4(child)",           SAME_AS,  12 },
    [16] = { "seccomp(allow all)",     EQ,       0 },
    [17] = { "  open() still",         EQ,       -1 },      // only stricter
    [18] = { "seccomp(STRICT)",        EQ,       0 },
    [19] = { "getpid() ended us",      EQ,       0 },       // SIGSYS
};

static const check_t debug_checks[] = {
    [1]  = { "clone(child: pause)",    NEW_TASK, 0 },
    [2]  = { "ptrace(ATTACH, child)",  EQ,       0 },
    [3]  = { "ptrace(ATTACH, 1)",      EQ,       -1 },      // ptrace_restrict
    [4]  = { "ptrace(PEEKDATA)",       EQ,       0 },
    [5]  = { "  word read",            EQ,       0x1234 },
    [6]  = { "ptrace(POKEDATA)",       EQ,       0 },
    [7]  = { "  word now",             EQ,       0x5678 },
    [8]  = { "ptrace(GETREGS)",        EQ,       0 },
    [9]  = { "  orig_eax: pause",      EQ,       78 },
    [10] = { "  eip in user code",     AT_LEAST, 0x40000000 },
    [11] = { "PEEKDATA(kernel)",       EQ,       -1 },      // vm_access_check
    [12] = { "ptrace(DETACH)",         EQ,       0 },
    [13] = { "PEEKDATA after detach",  EQ,       -1 },
    [14] = { "ptrace(ATTACH) again",   EQ,       0 },
    [15] = { "ptrace(KILL)",           EQ,       0 },
    [16] = { "wait4(child)",           SAME_AS,  1 },
    [17] = { "  status: SIGKILL",      EQ,       9 },
    [18] = { "process_vm_writev(me)",  EQ,       4 },
    [19] = { "process_vm_readv(me)",   EQ,       4 },
    [20] = { "  bytes \"abcd\"",       EQ,       0x64636261 },
    [21] = { "process_vm_readv(1)",    EQ,       -1 },      // vm_access_check
    [22] = { "kcmp(me, me, VM)",       EQ,       0 },
    [23] = { "kcmp(me, me, FILES)",    EQ,       0 },
    [24] = { "kcmp(me, 1, VM)",        EQ,       -1 },      // hidepid
    [25] = { "kcmp(type 99)",          EQ,       -1 },
    [26] = { "  child: TRACEME",       EQ,       0 },
    [27] = { "wait4(child 2)",         NEW_TASK, 0 },
};

static const check_t exec_checks[] = {
    [1]  = { "open(secret, CLOEXEC)",  FD,       0 },
    [2]  = { "write(secret)",          EQ,       1 },
    [3]  = { "execve(no such file)",   EQ,       -1 },
    [4]  = { "execve(41 arguments)",   EQ,       -1 },      // arg_limits
    [5]  = { "execve(argv -> kernel)", EQ,       -1 },      // arg_limits
    [6]  = { "execve(not an ELF)",     EQ,       -1 },      // elf_validate
    [7]  = { "vfork() + execve",       NEW_TASK, 0 },
    [8]  = { "wait4(child)",           SAME_AS,  7 },
    [9]  = { "  status: exit 42",      EQ,       42 << 8 },
    [10] = { "  new program: argc",    EQ,       3 },
    [11] = { "  argv[1] \"one\"",      EQ,       0x00656E6F },
    [12] = { "  envc (2 dropped)",     EQ,       1 },       // env_sanitize
    [13] = { "  envp[0] \"HOME\"",     EQ,       0x454D4F48 },
    [14] = { "  read(CLOEXEC fd)",     EQ,       -1 },      // close_on_exec
    [15] = { "  getpid: same pid",     SAME_AS,  7 },
    [16] = { "fork()",                 EQ,       -1 },      // one address space
    [17] = { "execveat(fd 5, rel)",    EQ,       -1 },
};

static const struct {
    const char    *name;
    const uint8_t *start, *end;
    const check_t *checks;
    int            count;           // slots 1..count
} groups[] = {
    { "signals", signals_program, signals_program_end, signal_checks,
      sizeof(signal_checks) / sizeof(signal_checks[0]) - 1 },
    { "prctl", prctl_program, prctl_program_end, prctl_checks,
      sizeof(prctl_checks) / sizeof(prctl_checks[0]) - 1 },
    { "wait", wait_program, wait_program_end, wait_checks,
      sizeof(wait_checks) / sizeof(wait_checks[0]) - 1 },
    { "session", session_program, session_program_end, session_checks,
      sizeof(session_checks) / sizeof(session_checks[0]) - 1 },
    { "timers", timers_program, timers_program_end, timer_checks,
      sizeof(timer_checks) / sizeof(timer_checks[0]) - 1 },
    { "sandbox", sandbox_program, sandbox_program_end, sandbox_checks,
      sizeof(sandbox_checks) / sizeof(sandbox_checks[0]) - 1 },
    { "debug", debug_program, debug_program_end, debug_checks,
      sizeof(debug_checks) / sizeof(debug_checks[0]) - 1 },
    { "exec", exec_program, exec_program_end, exec_checks,
      sizeof(exec_checks) / sizeof(exec_checks[0]) - 1 },
};
#define GROUP_COUNT (int)(sizeof(groups) / sizeof(groups[0]))

static int as_expected(const check_t *c, int32_t got, int32_t pid, volatile uint32_t *box) {
    switch (c->expect) {
        case PID:      return got == pid;
        case FD:       return got >= 3;
        case AT_LEAST: return got >= c->value;
        case SAME_AS:  return got == (int32_t)box[c->value];
        case NEW_TASK: return got > pid;
        default:       return got == c->value;
    }
}

static int total_good, total_count;    // over every group run by one command

// The "exec" group runs /home/guest/showargs: put the ELF bytes there.
static void write_showargs(void) {
    int fd = sys_open(SHOWARGS_PATH, O_WRITE | O_CREATE | O_TRUNC);
    if (fd >= 0) {
        sys_write(fd, showargs_elf, showargs_elf_end - showargs_elf);
        sys_close(fd);
    }
}

// Run one group's program and print its answers. Returns 1 if all were right.
static int run_group(int g) {
    volatile uint32_t *box = usertask_mailbox();
    if (strcmp(groups[g].name, "exec") == 0)
        write_showargs();
    int pid = usertask_start(groups[g].name, groups[g].start, groups[g].end, GUEST_UID);
    if (pid < 0) {
        ui_error("proccalls", "could not start the ring 3 program");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != DONE) {
        ui_error("proccalls", "the program did not finish");
        return 0;
    }

    int good = 0, count = groups[g].count;
    ui_printf("\n%s: ring 3 program PID %d, user guest\n", groups[g].name, pid);
    for (int slot = 1; slot <= count; slot++) {
        const check_t *c = &groups[g].checks[slot];
        int32_t got = (int32_t)box[slot];
        int ok = as_expected(c, got, pid, box);
        good += ok;

        ui_printf("%-23s%6d ", c->call, got);
        ui_color(ok ? UI_GOOD : UI_BAD);
        vga_print_str(ok ? "ok" : "NO");
        ui_reset();
        vga_print_str(slot % 2 ? "    " : "\n");      // two columns
    }
    vga_print_str(count % 2 ? "\n" : "");
    ui_start(good == count ? UI_OK : UI_ERROR, "proccalls");
    ui_printf("%s: %d of %d answers as expected", groups[g].name, good, count);
    ui_end();
    total_good += good;
    total_count += count;
    return good == count;
}

void cmd_proccalls(const char *args) {
    int ran = 0;
    total_good = total_count = 0;
    for (int g = 0; g < GROUP_COUNT; g++) {
        if (args[0] && strcmp(args, groups[g].name) != 0)
            continue;
        run_group(g);
        ran++;
    }
    if (!ran) {
        ui_usage("proccalls [signals|prctl|wait|session|timers|sandbox|debug|exec]");
    } else if (ran > 1) {
        ui_start(total_good == total_count ? UI_OK : UI_ERROR, "proccalls");
        ui_printf("all %d groups: %d of %d answers as expected", ran, total_good, total_count);
        ui_end();
    }
}
