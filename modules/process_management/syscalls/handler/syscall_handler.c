/*
 * syscall_handler.c - Turn an int 0x80 into a call to the right sys_*().
 *
 * The CPU enters isr_handler() (arch/x86/isr) with all registers saved in
 * 'regs'. The syscall number is in EAX, arguments in EBX, ECX, EDX, ESI,
 * EDI, EBP (in that order, as Linux on 32-bit x86). The result goes back
 * into regs->eax: when the interrupt returns, that is what the program
 * sees in EAX.
 *
 * The table below has one entry per syscall. Each entry's small wrapper
 * takes the arguments out of the registers. When the call comes from
 * ring 3, every pointer argument is checked first (user_ptr_bounds): it
 * must be the program's own, mapped memory. A bad pointer then fails
 * just that call instead of crashing the kernel.
 *
 * Uses: register_interrupt_handler (arch/x86/isr), the sys_* functions of
 *       every module, and def_user_range_ok / def_user_string_ok
 *       (memory_management defence user_ptr_bounds), ui_printf (ui) for
 *       the trace.
 */

#include "syscall_handler.h"
#include "isr.h"
#include "ui.h"
#include "vfs.h"
#include "def_user_ptr_bounds.h"

// process_management
#include "sys_exit.h"
#include "sys_sleep.h"
// memory_management
#include "sys_sbrk.h"
#include "sys_mmap.h"
#include "sys_munmap.h"
// device_management
#include "sys_uptime.h"
#include "sys_time.h"
// filesystem_management
#include "sys_mkdir.h"
#include "sys_unlink.h"
#include "sys_chdir.h"
#include "sys_getcwd.h"
#include "sys_rename.h"
// io_management
#include "sys_open.h"
#include "sys_read.h"
#include "sys_write.h"
#include "sys_close.h"
#include "sys_seek.h"
// cpu_management (modules/cpu_management/syscalls/<name>/)
#include "modules/cpu_management/syscalls/getpid/sys_getpid.h"
#include "modules/cpu_management/syscalls/gettid/sys_gettid.h"
#include "modules/cpu_management/syscalls/sched_yield/sys_sched_yield.h"
#include "modules/cpu_management/syscalls/cputime/sys_cputime.h"
#include "modules/cpu_management/syscalls/setpriority/sys_setpriority.h"
#include "modules/cpu_management/syscalls/getpriority/sys_getpriority.h"
#include "modules/cpu_management/syscalls/sched_setscheduler/sys_sched_setscheduler.h"
#include "modules/cpu_management/syscalls/sched_getscheduler/sys_sched_getscheduler.h"
#include "modules/cpu_management/syscalls/sched_setparam/sys_sched_setparam.h"
#include "modules/cpu_management/syscalls/sched_getparam/sys_sched_getparam.h"
#include "modules/cpu_management/syscalls/sched_get_priority_min/sys_sched_get_priority_min.h"
#include "modules/cpu_management/syscalls/sched_get_priority_max/sys_sched_get_priority_max.h"
#include "modules/cpu_management/syscalls/sched_rr_get_interval/sys_sched_rr_get_interval.h"
#include "modules/cpu_management/syscalls/sched_setaffinity/sys_sched_setaffinity.h"
#include "modules/cpu_management/syscalls/sched_getaffinity/sys_sched_getaffinity.h"
#include "modules/cpu_management/syscalls/sched_setattr/sys_sched_setattr.h"
#include "modules/cpu_management/syscalls/sched_getattr/sys_sched_getattr.h"
#include "modules/cpu_management/syscalls/getrlimit/sys_getrlimit.h"
#include "modules/cpu_management/syscalls/setrlimit/sys_setrlimit.h"
#include "modules/cpu_management/syscalls/prlimit64/sys_prlimit64.h"
#include "modules/cpu_management/syscalls/getcpu/sys_getcpu.h"
#include "modules/cpu_management/syscalls/perf_event_open/sys_perf_event_open.h"
#include "modules/cpu_management/syscalls/clone/sys_clone.h"
#include "modules/cpu_management/syscalls/clone3/sys_clone3.h"
#include "modules/cpu_management/syscalls/move_pages/sys_move_pages.h"
#include "modules/cpu_management/syscalls/mbind/sys_mbind.h"
#include "modules/cpu_management/syscalls/set_mempolicy/sys_set_mempolicy.h"
#include "modules/cpu_management/syscalls/get_mempolicy/sys_get_mempolicy.h"
#include "modules/cpu_management/syscalls/migrate_pages/sys_migrate_pages.h"
#include "modules/cpu_management/syscalls/membarrier/sys_membarrier.h"
#include "modules/cpu_management/syscalls/exit_group/sys_exit_group.h"
// user_management
#include "sys_getuid.h"
#include "sys_setuid.h"
// security_management
#include "sys_getcaps.h"

#define SYSCALL_ERROR ((uint32_t)-1)

/* ---------- Checking pointers from ring 3 ---------- */

static int from_user(registers_t *r) {
    return (r->cs & 3) == 3;            // low 2 bits of CS = the caller's ring
}

// A buffer argument: fine from the kernel, checked from ring 3.
static int buffer_ok(registers_t *r, uint32_t addr, uint32_t size) {
    return !from_user(r) || def_user_range_ok(addr, size);
}

// A pointer that may also be 0 ("not wanted").
static int optional_ok(registers_t *r, uint32_t addr, uint32_t size) {
    return addr == 0 || buffer_ok(r, addr, size);
}

// A path argument: fine from the kernel, checked from ring 3.
static int path_ok(registers_t *r, uint32_t addr) {
    return !from_user(r) || def_user_string_ok(addr, FS_PATH_MAX);
}

/* ---------- One wrapper per syscall ---------- */

static uint32_t do_exit(registers_t *r)   { (void)r; sys_exit(); }
static uint32_t do_sleep(registers_t *r)  { sys_sleep(r->ebx); return 0; }

static uint32_t do_sbrk(registers_t *r)   { return (uint32_t)sys_sbrk((int32_t)r->ebx); }
static uint32_t do_mmap(registers_t *r)   { return (uint32_t)sys_mmap(r->ebx); }
static uint32_t do_munmap(registers_t *r) {
    if (!buffer_ok(r, r->ebx, r->ecx)) return SYSCALL_ERROR;
    return sys_munmap((void *)r->ebx, r->ecx);
}

static uint32_t do_uptime(registers_t *r) { (void)r; return sys_uptime(); }
static uint32_t do_time(registers_t *r)   { (void)r; return sys_time(); }

static uint32_t do_mkdir(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return sys_mkdir((const char *)r->ebx);
}
static uint32_t do_unlink(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return sys_unlink((const char *)r->ebx);
}
static uint32_t do_chdir(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return sys_chdir((const char *)r->ebx);
}
static uint32_t do_getcwd(registers_t *r) {
    if (!buffer_ok(r, r->ebx, r->ecx)) return SYSCALL_ERROR;
    return sys_getcwd((char *)r->ebx, r->ecx);
}
static uint32_t do_rename(registers_t *r) {
    if (!path_ok(r, r->ebx) || !path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return sys_rename((const char *)r->ebx, (const char *)r->ecx);
}

static uint32_t do_open(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return sys_open((const char *)r->ebx, r->ecx);
}
static uint32_t do_read(registers_t *r) {
    if (!buffer_ok(r, r->ecx, r->edx)) return SYSCALL_ERROR;
    return sys_read(r->ebx, (void *)r->ecx, r->edx);
}
static uint32_t do_write(registers_t *r) {
    if (!buffer_ok(r, r->ecx, r->edx)) return SYSCALL_ERROR;
    return sys_write(r->ebx, (const void *)r->ecx, r->edx);
}
static uint32_t do_close(registers_t *r) { return sys_close(r->ebx); }
static uint32_t do_seek(registers_t *r)  { return sys_seek(r->ebx, (int32_t)r->ecx, r->edx); }

/* cpu_management: identity and turns */
static uint32_t do_getpid(registers_t *r)      { (void)r; return sys_getpid(); }
static uint32_t do_gettid(registers_t *r)      { (void)r; return sys_gettid(); }
static uint32_t do_sched_yield(registers_t *r) { (void)r; return sys_sched_yield(); }
static uint32_t do_exit_group(registers_t *r)  { sys_exit_group((int)r->ebx); }
static uint32_t do_cputime(registers_t *r)     { return (uint32_t)sys_cputime(r->ebx); }

/* cpu_management: nice, policies, slices */
static uint32_t do_setpriority(registers_t *r) {
    return (uint32_t)sys_setpriority((int)r->ebx, r->ecx, (int)r->edx);
}
static uint32_t do_getpriority(registers_t *r) {
    return (uint32_t)sys_getpriority((int)r->ebx, r->ecx);
}
static uint32_t do_sched_setscheduler(registers_t *r) {
    if (!buffer_ok(r, r->edx, sizeof(sched_param_t))) return SYSCALL_ERROR;
    return sys_sched_setscheduler(r->ebx, (int)r->ecx, (const sched_param_t *)r->edx);
}
static uint32_t do_sched_getscheduler(registers_t *r) {
    return sys_sched_getscheduler(r->ebx);
}
static uint32_t do_sched_setparam(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(sched_param_t))) return SYSCALL_ERROR;
    return sys_sched_setparam(r->ebx, (const sched_param_t *)r->ecx);
}
static uint32_t do_sched_getparam(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(sched_param_t))) return SYSCALL_ERROR;
    return sys_sched_getparam(r->ebx, (sched_param_t *)r->ecx);
}
static uint32_t do_sched_get_priority_min(registers_t *r) {
    return sys_sched_get_priority_min((int)r->ebx);
}
static uint32_t do_sched_get_priority_max(registers_t *r) {
    return sys_sched_get_priority_max((int)r->ebx);
}
static uint32_t do_sched_rr_get_interval(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(timespec_t))) return SYSCALL_ERROR;
    return sys_sched_rr_get_interval(r->ebx, (timespec_t *)r->ecx);
}
static uint32_t do_sched_setattr(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(sched_attr_t))) return SYSCALL_ERROR;
    return sys_sched_setattr(r->ebx, (const sched_attr_t *)r->ecx, r->edx);
}
static uint32_t do_sched_getattr(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(sched_attr_t))) return SYSCALL_ERROR;
    return sys_sched_getattr(r->ebx, (sched_attr_t *)r->ecx, r->edx, r->esi);
}

/* cpu_management: CPUs and NUMA nodes */
static uint32_t do_sched_setaffinity(registers_t *r) {
    if (!buffer_ok(r, r->edx, r->ecx)) return SYSCALL_ERROR;
    return sys_sched_setaffinity(r->ebx, r->ecx, (const uint8_t *)r->edx);
}
static uint32_t do_sched_getaffinity(registers_t *r) {
    if (!buffer_ok(r, r->edx, r->ecx)) return SYSCALL_ERROR;
    return sys_sched_getaffinity(r->ebx, r->ecx, (uint8_t *)r->edx);
}
static uint32_t do_getcpu(registers_t *r) {
    if (!optional_ok(r, r->ebx, 4) || !optional_ok(r, r->ecx, 4)) return SYSCALL_ERROR;
    return sys_getcpu((uint32_t *)r->ebx, (uint32_t *)r->ecx);
}
static uint32_t do_move_pages(registers_t *r) {
    uint32_t bytes = r->ecx * 4;
    if (r->ecx > 0x10000000 || !buffer_ok(r, r->edx, bytes) ||
        !optional_ok(r, r->esi, bytes) || !buffer_ok(r, r->edi, bytes)) return SYSCALL_ERROR;
    return sys_move_pages(r->ebx, r->ecx, (const uint32_t *)r->edx,
                          (const int32_t *)r->esi, (int32_t *)r->edi, r->ebp);
}
static uint32_t do_mbind(registers_t *r) {
    if (!optional_ok(r, r->esi, 4)) return SYSCALL_ERROR;
    return sys_mbind(r->ebx, r->ecx, (int)r->edx, (const uint32_t *)r->esi, r->edi, r->ebp);
}
static uint32_t do_set_mempolicy(registers_t *r) {
    if (!optional_ok(r, r->ecx, 4)) return SYSCALL_ERROR;
    return sys_set_mempolicy((int)r->ebx, (const uint32_t *)r->ecx, r->edx);
}
static uint32_t do_get_mempolicy(registers_t *r) {
    if (!optional_ok(r, r->ebx, 4) || !optional_ok(r, r->ecx, 4)) return SYSCALL_ERROR;
    return sys_get_mempolicy((int32_t *)r->ebx, (uint32_t *)r->ecx, r->edx, r->esi, r->edi);
}
static uint32_t do_migrate_pages(registers_t *r) {
    if (!buffer_ok(r, r->edx, 4) || !buffer_ok(r, r->esi, 4)) return SYSCALL_ERROR;
    return sys_migrate_pages(r->ebx, r->ecx, (const uint32_t *)r->edx, (const uint32_t *)r->esi);
}

/* cpu_management: limits */
static uint32_t do_getrlimit(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(rlimit_t))) return SYSCALL_ERROR;
    return sys_getrlimit((int)r->ebx, (rlimit_t *)r->ecx);
}
static uint32_t do_setrlimit(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(rlimit_t))) return SYSCALL_ERROR;
    return sys_setrlimit((int)r->ebx, (const rlimit_t *)r->ecx);
}
static uint32_t do_prlimit64(registers_t *r) {
    if (!optional_ok(r, r->edx, sizeof(rlimit64_t)) ||
        !optional_ok(r, r->esi, sizeof(rlimit64_t))) return SYSCALL_ERROR;
    return sys_prlimit64(r->ebx, (int)r->ecx, (const rlimit64_t *)r->edx, (rlimit64_t *)r->esi);
}

/* cpu_management: threads, counters, barriers */
static uint32_t do_clone(registers_t *r) {
    if (!optional_ok(r, r->edx, 4) || !optional_ok(r, r->edi, 4)) return SYSCALL_ERROR;
    return sys_clone(r, r->ebx, r->ecx, (uint32_t *)r->edx, (uint32_t *)r->edi);
}
static uint32_t do_clone3(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sizeof(clone_args_t))) return SYSCALL_ERROR;
    const clone_args_t *a = (const clone_args_t *)r->ebx;
    if ((a->parent_tid && !buffer_ok(r, (uint32_t)a->parent_tid, 4)) ||
        (a->child_tid && !buffer_ok(r, (uint32_t)a->child_tid, 4))) return SYSCALL_ERROR;
    return sys_clone3(r, a, r->ecx);
}
static uint32_t do_perf_event_open(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sizeof(perf_event_attr_t))) return SYSCALL_ERROR;
    return sys_perf_event_open((const perf_event_attr_t *)r->ebx, (int)r->ecx,
                               (int)r->edx, (int)r->esi, r->edi);
}
static uint32_t do_membarrier(registers_t *r) {
    return sys_membarrier((int)r->ebx, r->ecx, (int)r->edx);
}

static uint32_t do_getuid(registers_t *r) { (void)r; return sys_getuid(); }
static uint32_t do_setuid(registers_t *r) { return (uint32_t)sys_setuid(r->ebx); }

static uint32_t do_getcaps(registers_t *r) { (void)r; return sys_getcaps(); }

/* ---------- The table ---------- */

static const struct {
    uint32_t number;
    const char *name;
    uint32_t (*run)(registers_t *r);
} table[] = {
    { SYS_EXIT,   "exit",   do_exit   },
    { SYS_GETPID, "getpid", do_getpid },
    { SYS_SLEEP,  "sleep",  do_sleep  },
    { SYS_SCHED_YIELD, "sched_yield", do_sched_yield },
    { SYS_SBRK,   "sbrk",   do_sbrk   },
    { SYS_MMAP,   "mmap",   do_mmap   },
    { SYS_MUNMAP, "munmap", do_munmap },
    { SYS_UPTIME, "uptime", do_uptime },
    { SYS_MKDIR,  "mkdir",  do_mkdir  },
    { SYS_UNLINK, "unlink", do_unlink },
    { SYS_CHDIR,  "chdir",  do_chdir  },
    { SYS_GETCWD, "getcwd", do_getcwd },
    { SYS_RENAME, "rename", do_rename },
    { SYS_OPEN,   "open",   do_open   },
    { SYS_READ,   "read",   do_read   },
    { SYS_WRITE,  "write",  do_write  },
    { SYS_CLOSE,  "close",  do_close  },
    { SYS_SEEK,   "seek",   do_seek   },
    { SYS_TIME,   "time",   do_time   },
    { SYS_CPUTIME,     "cputime",     do_cputime     },
    { SYS_SETPRIORITY, "setpriority", do_setpriority },
    { SYS_GETPRIORITY, "getpriority", do_getpriority },
    { SYS_GETUID,      "getuid",      do_getuid      },
    { SYS_SETUID,      "setuid",      do_setuid      },
    { SYS_GETCAPS,     "getcaps",     do_getcaps     },
    { SYS_SCHED_SETSCHEDULER,     "sched_setscheduler",     do_sched_setscheduler     },
    { SYS_SCHED_GETSCHEDULER,     "sched_getscheduler",     do_sched_getscheduler     },
    { SYS_SCHED_SETPARAM,         "sched_setparam",         do_sched_setparam         },
    { SYS_SCHED_GETPARAM,         "sched_getparam",         do_sched_getparam         },
    { SYS_SCHED_GET_PRIORITY_MIN, "sched_get_priority_min", do_sched_get_priority_min },
    { SYS_SCHED_GET_PRIORITY_MAX, "sched_get_priority_max", do_sched_get_priority_max },
    { SYS_SCHED_RR_GET_INTERVAL,  "sched_rr_get_interval",  do_sched_rr_get_interval  },
    { SYS_SCHED_SETAFFINITY,      "sched_setaffinity",      do_sched_setaffinity      },
    { SYS_SCHED_GETAFFINITY,      "sched_getaffinity",      do_sched_getaffinity      },
    { SYS_SCHED_SETATTR,          "sched_setattr",          do_sched_setattr          },
    { SYS_SCHED_GETATTR,          "sched_getattr",          do_sched_getattr          },
    { SYS_GETRLIMIT,              "getrlimit",              do_getrlimit              },
    { SYS_SETRLIMIT,              "setrlimit",              do_setrlimit              },
    { SYS_PRLIMIT64,              "prlimit64",              do_prlimit64              },
    { SYS_GETCPU,                 "getcpu",                 do_getcpu                 },
    { SYS_PERF_EVENT_OPEN,        "perf_event_open",        do_perf_event_open        },
    { SYS_GETTID,                 "gettid",                 do_gettid                 },
    { SYS_CLONE,                  "clone",                  do_clone                  },
    { SYS_CLONE3,                 "clone3",                 do_clone3                 },
    { SYS_MOVE_PAGES,             "move_pages",             do_move_pages             },
    { SYS_MBIND,                  "mbind",                  do_mbind                  },
    { SYS_SET_MEMPOLICY,          "set_mempolicy",          do_set_mempolicy          },
    { SYS_GET_MEMPOLICY,          "get_mempolicy",          do_get_mempolicy          },
    { SYS_MIGRATE_PAGES,          "migrate_pages",          do_migrate_pages          },
    { SYS_MEMBARRIER,             "membarrier",             do_membarrier             },
    { SYS_EXIT_GROUP,             "exit_group",             do_exit_group             },
};
#define TABLE_SIZE (sizeof(table) / sizeof(table[0]))

// Show syscalls that come from user mode, so you can watch ring 3 work.
static void trace(const char *name) {
    uint8_t color = vga_get_color();        // put back whatever was in use
    ui_color(UI_DIM);
    ui_printf("[syscall] PID %d (ring 3) -> %s\n", sys_getpid(), name);
    vga_set_color_byte(color);
}

static void syscall_handler(registers_t *regs) {
    for (unsigned i = 0; i < TABLE_SIZE; i++) {
        if (table[i].number != regs->eax)
            continue;
        if (from_user(regs))
            trace(table[i].name);
        regs->eax = table[i].run(regs);
        return;
    }
    if (from_user(regs))
        trace("unknown");
    regs->eax = SYSCALL_ERROR;          // no such syscall
}

void init_syscalls(void) {
    register_interrupt_handler(SYSCALL_VECTOR, syscall_handler);
}
