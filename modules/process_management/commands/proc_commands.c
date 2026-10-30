/*
 * proc_commands.c - Register the process commands with the shell.
 *
 * One row per command: the name you type, the line `help` shows, and
 * the function in commands/<name>/cmd_<name>.c that runs it.
 *
 * Uses: shell_begin_group / shell_register_command (shell), and every
 *       cmd_<name>() of this module.
 */

#include "modules/process_management/commands/proc_commands.h"  // this file
#include "modules/shell/core/shell/shell.h"                     // shell_register_command

// Starting and stopping
#include "modules/process_management/commands/kill/cmd_kill.h"
#include "modules/process_management/commands/spawn/cmd_spawn.h"
#include "modules/process_management/commands/sleep/cmd_sleep.h"
#include "modules/process_management/commands/exec/cmd_exec.h"
#include "modules/process_management/commands/testuser/cmd_testuser.h"
// Looking at processes
#include "modules/process_management/commands/procinfo/cmd_procinfo.h"
#include "modules/process_management/commands/pstree/cmd_pstree.h"
#include "modules/process_management/commands/pgrep/cmd_pgrep.h"
#include "modules/process_management/commands/pidof/cmd_pidof.h"
#include "modules/process_management/commands/pwdx/cmd_pwdx.h"
// Signalling by name
#include "modules/process_management/commands/pkill/cmd_pkill.h"
#include "modules/process_management/commands/killall/cmd_killall.h"
// Jobs and sessions
#include "modules/process_management/commands/jobs/cmd_jobs.h"
#include "modules/process_management/commands/fg/cmd_fg.h"
#include "modules/process_management/commands/bg/cmd_bg.h"
#include "modules/process_management/commands/wait/cmd_wait.h"
#include "modules/process_management/commands/nohup/cmd_nohup.h"
#include "modules/process_management/commands/setsid/cmd_setsid.h"
#include "modules/process_management/commands/timeout/cmd_timeout.h"
// Debugging
#include "modules/process_management/commands/strace/cmd_strace.h"
#include "modules/process_management/commands/pmap/cmd_pmap.h"
// Testing the syscalls
#include "modules/process_management/commands/proccalls/cmd_proccalls.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "kill",     "Kill or signal: kill [-SIG] <pid> | kill -l", cmd_kill },
    { "spawn",    "Start a background worker process",      cmd_spawn    },
    { "sleep",    "Sleep for N seconds",                    cmd_sleep    },
    { "exec",     "Run an ELF program: exec <file> [&]",    cmd_exec     },
    { "testuser", "Ring 3 program: testuser [null|badptr]", cmd_testuser },
    { "procinfo", "Everything about a process: procinfo [pid]", cmd_procinfo },
    { "pstree",   "Process family tree: pstree [-p] [pid]",     cmd_pstree   },
    { "pgrep",    "Find PIDs by name: pgrep [-l] [-x] [-u user] pattern", cmd_pgrep },
    { "pidof",    "PIDs of a program: pidof name...",           cmd_pidof    },
    { "pwdx",     "Working directory of a process: pwdx pid...", cmd_pwdx    },
    { "pkill",    "Signal by name: pkill [-SIG] [-x] [-u user] pattern", cmd_pkill },
    { "killall",  "Signal all with a name: killall [-SIG] name", cmd_killall },
    { "jobs",     "The shell's jobs, running or stopped (Ctrl+Z stops one)", cmd_jobs },
    { "fg",       "Job to the foreground: fg [%job]",           cmd_fg       },
    { "bg",       "Stopped job carries on in the background: bg [%job]", cmd_bg },
    { "wait",     "Wait for jobs to end: wait [%job | pid]...",  cmd_wait     },
    { "nohup",    "Run, immune to logout's SIGHUP: nohup command", cmd_nohup  },
    { "setsid",   "Run in a new session: setsid command",       cmd_setsid   },
    { "timeout",  "Time limit: timeout [-s SIG] seconds command", cmd_timeout },
    { "strace",   "Show a program's syscalls: strace command | -p pid", cmd_strace },
    { "pmap",     "Memory map of a process: pmap [pid]",        cmd_pmap     },
    { "proccalls", "Test: a ring 3 program calls the process syscalls", cmd_proccalls },
};

void register_proc_commands(void) {
    shell_begin_group("Processes");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}
