/*
 * cpu_commands.c - Register the CPU commands with the shell.
 *
 * One row per command: the name you type, the line `help` shows, and
 * the function in commands/<name>/cmd_<name>.c that runs it.
 *
 * Uses: shell_begin_group / shell_register_command (shell), and every
 *       cmd_<name>() of this module.
 */

#include "modules/cpu_management/commands/cpu_commands.h"                   // this file
#include "modules/shell/core/shell/shell.h"                                 // shell_register_command

// UzOS's own views
#include "modules/cpu_management/commands/cpuinfo/cmd_cpuinfo.h"
#include "modules/cpu_management/commands/faults/cmd_faults.h"
#include "modules/cpu_management/commands/cpustat/cmd_cpustat.h"
#include "modules/cpu_management/commands/cpucalls/cmd_cpucalls.h"
// The CPU information tools
#include "modules/cpu_management/commands/lscpu/cmd_lscpu.h"
#include "modules/cpu_management/commands/nproc/cmd_nproc.h"
#include "modules/cpu_management/commands/getconf/cmd_getconf.h"
// Watching the CPU
#include "modules/cpu_management/commands/top/cmd_top.h"
#include "modules/cpu_management/commands/htop/cmd_htop.h"
#include "modules/cpu_management/commands/btop/cmd_btop.h"
#include "modules/cpu_management/commands/ps/cmd_ps.h"
#include "modules/cpu_management/commands/uptime/cmd_uptime.h"
#include "modules/cpu_management/commands/mpstat/cmd_mpstat.h"
#include "modules/cpu_management/commands/vmstat/cmd_vmstat.h"
#include "modules/cpu_management/commands/pidstat/cmd_pidstat.h"
#include "modules/cpu_management/commands/sar/cmd_sar.h"
#include "modules/cpu_management/commands/systemd_cgtop/cmd_systemd_cgtop.h"
#include "modules/cpu_management/commands/systemctl/cmd_systemctl.h"
// Changing how processes get the CPU
#include "modules/cpu_management/commands/nice/cmd_nice.h"
#include "modules/cpu_management/commands/renice/cmd_renice.h"
#include "modules/cpu_management/commands/chrt/cmd_chrt.h"
#include "modules/cpu_management/commands/taskset/cmd_taskset.h"
#include "modules/cpu_management/commands/numactl/cmd_numactl.h"
#include "modules/cpu_management/commands/ulimit/cmd_ulimit.h"
// Measuring
#include "modules/cpu_management/commands/time/cmd_time.h"
#include "modules/cpu_management/commands/perf/cmd_perf.h"
// Power
#include "modules/cpu_management/commands/cpupower/cmd_cpupower.h"
#include "modules/cpu_management/commands/turbostat/cmd_turbostat.h"
#include "modules/cpu_management/commands/powerprofilesctl/cmd_powerprofilesctl.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "cpuinfo",  "The CPU: name, features, control registers: cpuinfo [flags]", cmd_cpuinfo },
    { "faults",   "CPU exceptions that killed a process: faults [list]",         cmd_faults  },
    { "cpustat",  "CPU load: busy/idle, ring 0/3, time per process",             cmd_cpustat },
    { "cpucalls", "Test: a ring 3 program calls every CPU syscall",              cmd_cpucalls },
    { "lscpu",    "CPU architecture, model, flags, NUMA: lscpu [-e]",            cmd_lscpu   },
    { "nproc",    "Processing units available: nproc [--all]",                   cmd_nproc   },
    { "getconf",  "System values: getconf _NPROCESSORS_ONLN | -a",               cmd_getconf },
    { "top",      "Live CPU and processes ('q' quits): top [-b] [-d s] [-n N]",  cmd_top     },
    { "htop",     "Interactive process viewer: select, renice, kill",            cmd_htop    },
    { "btop",     "Resource monitor: CPU graph, memory, processes",              cmd_btop    },
    { "ps",       "Process snapshot: ps [-e] [-f] [-L] [aux] [-o fields]",       cmd_ps      },
    { "uptime",   "Up time, users, load average: uptime [-p|-s]",                cmd_uptime  },
    { "mpstat",   "CPU use per CPU: mpstat [-P ALL] [interval [count]]",         cmd_mpstat  },
    { "vmstat",   "Procs, memory, irq, switches, CPU: vmstat [interval [count]]", cmd_vmstat },
    { "pidstat",  "CPU use per process: pidstat [-w] [-p PID] [interval]",       cmd_pidstat },
    { "sar",      "System activity: sar [-u|-q|-w] [interval [count]]",          cmd_sar     },
    { "systemd-cgtop", "CPU and memory per slice: systemd-cgtop [-n N]",         cmd_systemd_cgtop },
    { "systemctl", "Services and their settings: systemctl [status|show UNIT]",  cmd_systemctl },
    { "nice",     "Run with a niceness: nice [-n N] command",                    cmd_nice    },
    { "renice",   "Change niceness: renice N -p PID | -u USER",                  cmd_renice  },
    { "chrt",     "Scheduling policy: chrt -p PID | chrt -f 10 command",         cmd_chrt    },
    { "taskset",  "CPU affinity: taskset -p PID | taskset -c 0 command",         cmd_taskset },
    { "numactl",  "NUMA: numactl --hardware | --membind=0 command",              cmd_numactl },
    { "ulimit",   "Resource limits of the shell: ulimit -a | -t 2",              cmd_ulimit  },
    { "time",     "Run a command, show real/user/sys: time command",             cmd_time    },
    { "perf",     "Counters and sampling: perf stat|record|report|list",         cmd_perf    },
    { "cpupower", "Frequency and idle states: cpupower frequency-info",          cmd_cpupower },
    { "turbostat", "MHz, busy %, IRQs, idle per interval: turbostat -i 1",       cmd_turbostat },
    { "powerprofilesctl", "Power profile: powerprofilesctl [list|get|set NAME]", cmd_powerprofilesctl },
};

void register_cpu_commands(void) {
    shell_begin_group("CPU");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}
