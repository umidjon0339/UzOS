/*
 * cmd_procinfo.c - `procinfo [pid]`: everything the kernel knows about
 * one process, read straight from its process_t.
 *
 *   PID 5  worker  S (sleeping)  user root
 *   family   parent 2  group 2  session 2  thread group 5
 *   time     started 12 s ago, CPU 0.01 s user + 0.03 s kernel
 *   sched    nice 0, priority 4, policy 0
 *   ---- details: its owner and root only (hidepid) ----
 *   cwd      /home/root
 *   files    3 open: 0 1 2
 *   signals  pending 00000000  blocked 00000000  ignored ...  caught ...
 *   timers   none
 *   altstack none
 *   prctl    pdeathsig 0  dumpable 1  no_new_privs 0  subreaper 0
 *   kernel   stack 00123000  esp 00123F80  entry 00104A2C (kptr_hide)
 *
 * The first lines are for everyone (`ps` shows them too). The details
 * are asked of hidepid, and every kernel address goes through kptr_hide,
 * so a normal user sees zeros there.
 *
 * Uses: process_get_by_pid / process_caller_uid / scheduler_get_current_process
 *       (core/process, core/scheduler), vfs_path_of (filesystem_management),
 *       users_name (user_management), timer_ticks / TIMER_HZ (device_management),
 *       def_hidepid_visible, def_kptr_show (security/), atoi, ui_*.
 */

#include "modules/process_management/commands/procinfo/cmd_procinfo.h"    // this file
#include "modules/process_management/core/process/process.h"              // process_get_by_pid, process_caller_uid
#include "modules/process_management/core/scheduler/scheduler.h"          // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                // SIG_IGN, SIG_DFL
#include "modules/process_management/security/proc_info_leak/defence/hidepid/def_hidepid.h"       // def_hidepid_visible
#include "modules/process_management/security/kernel_pointer_leak/defence/kptr_hide/def_kptr_hide.h"  // def_kptr_show
#include "modules/filesystem_management/core/vfs/vfs.h"                   // vfs_path_of, FS_PATH_MAX
#include "modules/user_management/core/users/users.h"                     // users_name
#include "modules/device_management/timer/timer.h"                        // timer_ticks, TIMER_HZ
#include "lib/stdlib/stdlib.h"                                            // atoi
#include "modules/ui/core/style/ui.h"                                     // ui_printf, ui_color, ui_error

static const char *state_text(process_state_t s) {
    switch (s) {
        case PROCESS_STATE_RUNNING:  return "R (running)";
        case PROCESS_STATE_READY:    return "R (ready)";
        case PROCESS_STATE_SLEEPING: return "S (sleeping)";
        case PROCESS_STATE_ZOMBIE:   return "Z (zombie)";
        case PROCESS_STATE_STOPPED:  return "T (stopped)";
        default:                     return "? (free)";
    }
}

static void label(const char *text) {
    ui_color(UI_DIM);
    ui_printf("%-9s", text);
    ui_reset();
}

// CPU ticks as "1.23 s".
static void print_seconds(uint32_t ticks) {
    uint32_t hundredths = ticks * 100 / TIMER_HZ;
    ui_printf("%u.%02u s", hundredths / 100, hundredths % 100);
}

static void public_part(const process_t *p) {
    char user[USER_NAME_MAX + 1];
    users_name(p->uid, user, sizeof(user));

    ui_color(UI_ACCENT);
    ui_printf("PID %u  %s", p->pid, p->name);
    ui_reset();
    ui_printf("  %s  user %s (%u)%s\n", state_text(p->state), user, p->uid,
              p->is_system ? "  [system]" : "");

    label("family");
    ui_printf("parent %u  group %u  session %u  thread group %u\n",
              p->ppid, p->pgid, p->sid, p->tgid ? p->tgid : p->pid);
    label("time");
    ui_printf("started %u s ago, CPU ", (timer_ticks - p->start_tick) / TIMER_HZ);
    print_seconds(p->user_ticks);
    ui_printf(" user + ");
    print_seconds(p->kernel_ticks);
    ui_printf(" kernel\n");
    label("sched");
    ui_printf("nice %d, priority %d, policy %d\n", p->nice, p->priority, p->policy);
    if (p->state == PROCESS_STATE_ZOMBIE) {
        label("ended");
        if (p->exit_status & 0x7F)
            ui_printf("by signal %d%s\n", p->exit_status & 0x7F,
                      p->exit_status & 0x80 ? " (core dumped)" : "");
        else
            ui_printf("exit code %d\n", (p->exit_status >> 8) & 0xFF);
    }
}

static void private_part(const process_t *p, uint32_t viewer) {
    char path[FS_PATH_MAX];

    label("cwd");
    if (p->cwd && vfs_path_of(p->cwd, path, sizeof(path)) == 0)
        ui_printf("%s\n", path);
    else
        ui_printf("/\n");

    int open = 0;
    for (int fd = 0; fd < MAX_FDS; fd++)
        open += p->fd_table[fd] != 0;
    label("files");
    ui_printf("%d open:", open);
    for (int fd = 0; fd < MAX_FDS; fd++)
        if (p->fd_table[fd])
            ui_printf(" %d", fd);
    ui_printf("\n");

    uint32_t ignored = 0, caught = 0;
    for (int sig = 1; sig < SIGNAL_COUNT; sig++) {
        if (p->sig_handler[sig] == SIG_IGN)       ignored |= SIG_BIT(sig);
        else if (p->sig_handler[sig] != SIG_DFL)  caught  |= SIG_BIT(sig);
    }
    label("signals");
    ui_printf("pending %08x  blocked %08x  ignored %08x  caught %08x\n",
              p->signals, p->sig_blocked, ignored, caught);

    static const char *const timer_names[ITIMERS] = { "real", "virtual", "prof" };
    label("timers");
    int armed = 0;
    for (int t = 0; t < ITIMERS; t++) {
        if (!p->itimer_value[t])
            continue;
        ui_printf("%s%s %u ms", armed++ ? ", " : "", timer_names[t],
                  p->itimer_value[t] * (1000 / TIMER_HZ));
        if (p->itimer_interval[t])
            ui_printf(" (every %u ms)", p->itimer_interval[t] * (1000 / TIMER_HZ));
    }
    ui_printf(armed ? "\n" : "none\n");
    label("altstack");
    if (p->altstack_size)
        ui_printf("%x, %u bytes\n", p->altstack_sp, p->altstack_size);
    else
        ui_printf("none\n");
    label("prctl");
    ui_printf("pdeathsig %d  dumpable %d  no_new_privs %d  subreaper %d\n",
              p->pdeathsig, p->dumpable, p->no_new_privs, p->child_subreaper);
    static const char *const modes[] = { "off", "strict", "filter" };
    label("seccomp");
    ui_printf("%s\n", p->seccomp_mode <= 2 ? modes[p->seccomp_mode] : "?");

    // DEFENCE kptr_hide: kernel addresses only for root.
    label("kernel");
    ui_printf("stack %08x  esp %08x  entry %08x\n",
              def_kptr_show(viewer, p->stack_base),
              def_kptr_show(viewer, p->esp),
              def_kptr_show(viewer, (uint32_t)p->entry));
}

void cmd_procinfo(const char *args) {
    process_t *self = scheduler_get_current_process();
    uint32_t pid = args[0] ? (uint32_t)atoi(args) : self->pid;
    process_t *p = process_get_by_pid(pid);
    if (!p) {
        ui_error("procinfo", "no such process (usage: procinfo [pid])");
        return;
    }

    uint32_t viewer = process_caller_uid();
    public_part(p);

    // DEFENCE hidepid: the rest is the owner's business.
    if (!def_hidepid_visible(viewer, p)) {
        ui_color(UI_DIM);
        ui_printf("(details hidden: not your process; see `audit`)\n");
        ui_reset();
        return;
    }
    private_part(p, viewer);
}
