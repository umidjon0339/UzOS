/*
 * syscall_handler.c - Turn an int 0x80 into a call to the right sys_*().
 *
 * The CPU enters isr_handler() (arch/x86/isr) with all registers saved in
 * 'regs'. The syscall number is in EAX, arguments in EBX, ECX, EDX. The
 * result goes back into regs->eax: when the interrupt returns, that is
 * what the program sees in EAX.
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
#include "sys_getpid.h"
#include "sys_sleep.h"
#include "sys_yield.h"
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
// cpu_management
#include "sys_cputime.h"
#include "sys_setpriority.h"
#include "sys_getpriority.h"

#define SYSCALL_ERROR ((uint32_t)-1)

/* ---------- Checking pointers from ring 3 ---------- */

static int from_user(registers_t *r) {
    return (r->cs & 3) == 3;            // low 2 bits of CS = the caller's ring
}

// A buffer argument: fine from the kernel, checked from ring 3.
static int buffer_ok(registers_t *r, uint32_t addr, uint32_t size) {
    return !from_user(r) || def_user_range_ok(addr, size);
}

// A path argument: fine from the kernel, checked from ring 3.
static int path_ok(registers_t *r, uint32_t addr) {
    return !from_user(r) || def_user_string_ok(addr, FS_PATH_MAX);
}

/* ---------- One wrapper per syscall ---------- */

static uint32_t do_exit(registers_t *r)   { (void)r; sys_exit(); }
static uint32_t do_getpid(registers_t *r) { (void)r; return sys_getpid(); }
static uint32_t do_sleep(registers_t *r)  { sys_sleep(r->ebx); return 0; }
static uint32_t do_yield(registers_t *r)  { (void)r; sys_yield(); return 0; }

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

static uint32_t do_cputime(registers_t *r)     { return (uint32_t)sys_cputime(r->ebx); }
static uint32_t do_setpriority(registers_t *r) { return (uint32_t)sys_setpriority(r->ebx, (int32_t)r->ecx); }
static uint32_t do_getpriority(registers_t *r) { return (uint32_t)sys_getpriority(r->ebx); }

/* ---------- The table ---------- */

static const struct {
    uint32_t number;
    const char *name;
    uint32_t (*run)(registers_t *r);
} table[] = {
    { SYS_EXIT,   "exit",   do_exit   },
    { SYS_GETPID, "getpid", do_getpid },
    { SYS_SLEEP,  "sleep",  do_sleep  },
    { SYS_YIELD,  "yield",  do_yield  },
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
