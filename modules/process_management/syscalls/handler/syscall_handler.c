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

#include "modules/process_management/syscalls/handler/syscall_handler.h"                     // this file
#include "arch/x86/isr/isr.h"                                                                // registers_t, register_interrupt_handler
#include "modules/ui/core/style/ui.h"                                                        // ui_color, UI_DIM, ui_printf
#include "modules/filesystem_management/core/vfs/vfs.h"                                      // FS_PATH_MAX
#include "modules/memory_management/security/defence/user_ptr_bounds/def_user_ptr_bounds.h"  // def_user_range_ok, def_user_string_ok

// process_management
#include "modules/process_management/syscalls/exit/sys_exit.h"                               // sys_exit, SYS_EXIT
#include "modules/process_management/syscalls/sleep/sys_sleep.h"                             // sys_sleep, SYS_SLEEP
// memory_management
#include "modules/memory_management/syscalls/sbrk/sys_sbrk.h"                                // SYS_SBRK
#include "modules/memory_management/syscalls/mmap/sys_mmap.h"                                // sys_mmap, SYS_MMAP
#include "modules/memory_management/syscalls/munmap/sys_munmap.h"                            // sys_munmap, SYS_MUNMAP
// device_management
#include "modules/device_management/syscalls/uptime/sys_uptime.h"                            // sys_uptime, SYS_UPTIME
#include "modules/device_management/syscalls/time/sys_time.h"                                // sys_time, SYS_TIME
// filesystem_management
#include "modules/filesystem_management/syscalls/mkdir/sys_mkdir.h"                          // sys_mkdir, SYS_MKDIR
#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"                        // sys_unlink, SYS_UNLINK
#include "modules/filesystem_management/syscalls/chdir/sys_chdir.h"                          // sys_chdir, SYS_CHDIR
#include "modules/filesystem_management/syscalls/getcwd/sys_getcwd.h"                        // sys_getcwd, SYS_GETCWD
#include "modules/filesystem_management/syscalls/rename/sys_rename.h"                        // sys_rename, SYS_RENAME
// filesystem_management: metadata (stat, rights, size, times)
#include "modules/filesystem_management/syscalls/stat/sys_stat.h"
#include "modules/filesystem_management/syscalls/lstat/sys_lstat.h"
#include "modules/filesystem_management/syscalls/fstat/sys_fstat.h"
#include "modules/filesystem_management/syscalls/fstatat/sys_fstatat.h"
#include "modules/filesystem_management/syscalls/statx/sys_statx.h"
#include "modules/filesystem_management/syscalls/access/sys_access.h"
#include "modules/filesystem_management/syscalls/faccessat/sys_faccessat.h"
#include "modules/filesystem_management/syscalls/faccessat2/sys_faccessat2.h"
#include "modules/filesystem_management/syscalls/chmod/sys_chmod.h"
#include "modules/filesystem_management/syscalls/fchmod/sys_fchmod.h"
#include "modules/filesystem_management/syscalls/fchmodat/sys_fchmodat.h"
#include "modules/filesystem_management/syscalls/chown/sys_chown.h"
#include "modules/filesystem_management/syscalls/fchown/sys_fchown.h"
#include "modules/filesystem_management/syscalls/lchown/sys_lchown.h"
#include "modules/filesystem_management/syscalls/fchownat/sys_fchownat.h"
#include "modules/filesystem_management/syscalls/umask/sys_umask.h"
#include "modules/filesystem_management/syscalls/truncate/sys_truncate.h"
#include "modules/filesystem_management/syscalls/ftruncate/sys_ftruncate.h"
#include "modules/filesystem_management/syscalls/fallocate/sys_fallocate.h"
#include "modules/filesystem_management/syscalls/utime/sys_utime.h"
#include "modules/filesystem_management/syscalls/utimes/sys_utimes.h"
#include "modules/filesystem_management/syscalls/utimensat/sys_utimensat.h"
// filesystem_management: directories
#include "modules/filesystem_management/syscalls/rmdir/sys_rmdir.h"
#include "modules/filesystem_management/syscalls/mkdirat/sys_mkdirat.h"
#include "modules/filesystem_management/syscalls/unlinkat/sys_unlinkat.h"
#include "modules/filesystem_management/syscalls/renameat/sys_renameat.h"
#include "modules/filesystem_management/syscalls/renameat2/sys_renameat2.h"
#include "modules/filesystem_management/syscalls/fchdir/sys_fchdir.h"
#include "modules/filesystem_management/syscalls/getdents/sys_getdents.h"
#include "modules/filesystem_management/syscalls/getdents64/sys_getdents64.h"
// filesystem_management: links
#include "modules/filesystem_management/syscalls/link/sys_link.h"
#include "modules/filesystem_management/syscalls/linkat/sys_linkat.h"
#include "modules/filesystem_management/syscalls/symlink/sys_symlink.h"
#include "modules/filesystem_management/syscalls/symlinkat/sys_symlinkat.h"
#include "modules/filesystem_management/syscalls/readlink/sys_readlink.h"
#include "modules/filesystem_management/syscalls/readlinkat/sys_readlinkat.h"
// io_management
#include "modules/io_management/syscalls/open/sys_open.h"                                    // sys_open, SYS_OPEN
#include "modules/io_management/syscalls/read/sys_read.h"                                    // sys_read, SYS_READ
#include "modules/io_management/syscalls/write/sys_write.h"                                  // sys_write, SYS_WRITE
#include "modules/io_management/syscalls/close/sys_close.h"                                  // sys_close, SYS_CLOSE
#include "modules/io_management/syscalls/seek/sys_seek.h"                                    // sys_seek, SYS_SEEK
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
#include "modules/process_management/syscalls/getppid/sys_getppid.h"
#include "modules/process_management/syscalls/setpgid/sys_setpgid.h"
#include "modules/process_management/syscalls/getpgid/sys_getpgid.h"
#include "modules/process_management/syscalls/getpgrp/sys_getpgrp.h"
#include "modules/process_management/syscalls/setsid/sys_setsid.h"
#include "modules/process_management/syscalls/getsid/sys_getsid.h"
#include "modules/process_management/syscalls/kill/sys_kill.h"
#include "modules/process_management/syscalls/tkill/sys_tkill.h"
#include "modules/process_management/syscalls/tgkill/sys_tgkill.h"
#include "modules/process_management/syscalls/pidfd_send_signal/sys_pidfd_send_signal.h"
#include "modules/process_management/syscalls/wait/sys_wait.h"
#include "modules/process_management/syscalls/waitpid/sys_waitpid.h"
#include "modules/process_management/syscalls/waitid/sys_waitid.h"
#include "modules/process_management/syscalls/fork/sys_fork.h"
#include "modules/process_management/syscalls/vfork/sys_vfork.h"
#include "modules/process_management/syscalls/execve/sys_execve.h"
#include "modules/process_management/syscalls/execveat/sys_execveat.h"
#include "modules/process_management/syscalls/rt_sigaction/sys_rt_sigaction.h"
#include "modules/process_management/syscalls/rt_sigprocmask/sys_rt_sigprocmask.h"
#include "modules/process_management/syscalls/rt_sigpending/sys_rt_sigpending.h"
#include "modules/process_management/syscalls/rt_sigsuspend/sys_rt_sigsuspend.h"
#include "modules/process_management/syscalls/rt_sigtimedwait/sys_rt_sigtimedwait.h"
#include "modules/process_management/syscalls/rt_sigqueueinfo/sys_rt_sigqueueinfo.h"
#include "modules/process_management/syscalls/rt_tgsigqueueinfo/sys_rt_tgsigqueueinfo.h"
#include "modules/process_management/syscalls/rt_sigreturn/sys_rt_sigreturn.h"
#include "modules/process_management/syscalls/sigaltstack/sys_sigaltstack.h"
#include "modules/process_management/syscalls/pause/sys_pause.h"
#include "modules/process_management/syscalls/alarm/sys_alarm.h"
#include "modules/process_management/syscalls/prctl/sys_prctl.h"
#include "modules/process_management/syscalls/wait4/sys_wait4.h"
#include "modules/process_management/syscalls/getrusage/sys_getrusage.h"
#include "modules/process_management/syscalls/times/sys_times.h"
#include "modules/process_management/syscalls/pidfd_open/sys_pidfd_open.h"
#include "modules/process_management/syscalls/pidfd_getfd/sys_pidfd_getfd.h"
#include "modules/process_management/syscalls/nanosleep/sys_nanosleep.h"
#include "modules/process_management/syscalls/clock_nanosleep/sys_clock_nanosleep.h"
#include "modules/process_management/syscalls/setitimer/sys_setitimer.h"
#include "modules/process_management/syscalls/getitimer/sys_getitimer.h"
#include "modules/process_management/syscalls/seccomp/sys_seccomp.h"
#include "modules/process_management/syscalls/unshare/sys_unshare.h"
#include "modules/process_management/syscalls/setns/sys_setns.h"
#include "modules/process_management/syscalls/ptrace/sys_ptrace.h"
#include "modules/process_management/syscalls/process_vm_readv/sys_process_vm_readv.h"
#include "modules/process_management/syscalls/process_vm_writev/sys_process_vm_writev.h"
#include "modules/process_management/syscalls/kcmp/sys_kcmp.h"
#include "modules/process_management/security/syscall_abuse/defence/seccomp_filter/def_seccomp_filter.h"  // def_seccomp_allowed
#include "modules/process_management/core/process/process.h"  // process_exit_with
#include "modules/process_management/core/scheduler/scheduler.h"  // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"  // SIGSYS
// user_management
#include "modules/user_management/syscalls/getuid/sys_getuid.h"                              // sys_getuid, SYS_GETUID
#include "modules/user_management/syscalls/setuid/sys_setuid.h"                              // sys_setuid, SYS_SETUID
// security_management
#include "modules/security_management/syscalls/getcaps/sys_getcaps.h"                        // sys_getcaps, SYS_GETCAPS

// network_management: sockets and names
#include "modules/network_management/syscalls/socket/sys_socket.h"
#include "modules/network_management/syscalls/bind/sys_bind.h"
#include "modules/network_management/syscalls/connect/sys_connect.h"
#include "modules/network_management/syscalls/listen/sys_listen.h"
#include "modules/network_management/syscalls/accept/sys_accept.h"
#include "modules/network_management/syscalls/accept4/sys_accept4.h"
#include "modules/network_management/syscalls/getsockname/sys_getsockname.h"
#include "modules/network_management/syscalls/getpeername/sys_getpeername.h"
#include "modules/network_management/syscalls/sendto/sys_sendto.h"
#include "modules/network_management/syscalls/recvfrom/sys_recvfrom.h"
#include "modules/network_management/syscalls/shutdown/sys_shutdown.h"
#include "modules/network_management/syscalls/setsockopt/sys_setsockopt.h"
#include "modules/network_management/syscalls/getsockopt/sys_getsockopt.h"
#include "modules/network_management/syscalls/socketcall/sys_socketcall.h"
#include "modules/network_management/syscalls/uname/sys_uname.h"
#include "modules/network_management/syscalls/sethostname/sys_sethostname.h"
#include "modules/network_management/syscalls/setdomainname/sys_setdomainname.h"
#include "modules/network_management/syscalls/gethostname/sys_gethostname.h"
#include "modules/network_management/syscalls/socketpair/sys_socketpair.h"
#include "modules/network_management/syscalls/sendmsg/sys_sendmsg.h"
#include "modules/network_management/syscalls/recvmsg/sys_recvmsg.h"
#include "modules/network_management/syscalls/sendmmsg/sys_sendmmsg.h"
#include "modules/network_management/syscalls/recvmmsg/sys_recvmmsg.h"
#include "modules/network_management/syscalls/select/sys_select.h"
#include "modules/network_management/syscalls/pselect6/sys_pselect6.h"
#include "modules/network_management/syscalls/poll/sys_poll.h"
#include "modules/network_management/syscalls/ppoll/sys_ppoll.h"
#include "modules/network_management/syscalls/epoll_create/sys_epoll_create.h"
#include "modules/network_management/syscalls/epoll_create1/sys_epoll_create1.h"
#include "modules/network_management/syscalls/epoll_ctl/sys_epoll_ctl.h"
#include "modules/network_management/syscalls/epoll_wait/sys_epoll_wait.h"
#include "modules/network_management/syscalls/epoll_pwait/sys_epoll_pwait.h"
#include "modules/network_management/syscalls/ioctl/sys_ioctl.h"
#include "modules/network_management/syscalls/sendfile/sys_sendfile.h"
#include "modules/network_management/core/inet/inet.h"                                       // NET_EFAULT, NET_EINVAL
#include "lib/string/string.h"                                                               // memcpy

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

static uint32_t do_exit(registers_t *r)   { sys_exit((int)r->ebx); }
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

/* filesystem_management: metadata (stat, rights, size, times) */
static uint32_t do_stat(registers_t *r) {
    if (!path_ok(r, r->ebx) || !buffer_ok(r, r->ecx, sizeof(vfs_stat_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_stat((const char *)r->ebx, (vfs_stat_t *)r->ecx);
}
static uint32_t do_lstat(registers_t *r) {
    if (!path_ok(r, r->ebx) || !buffer_ok(r, r->ecx, sizeof(vfs_stat_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_lstat((const char *)r->ebx, (vfs_stat_t *)r->ecx);
}
static uint32_t do_fstat(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(vfs_stat_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_fstat((int)r->ebx, (vfs_stat_t *)r->ecx);
}
static uint32_t do_fstatat(registers_t *r) {
    if (!path_ok(r, r->ecx) || !buffer_ok(r, r->edx, sizeof(vfs_stat_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_fstatat((int)r->ebx, (const char *)r->ecx, (vfs_stat_t *)r->edx, (int)r->esi);
}
static uint32_t do_statx(registers_t *r) {
    if (!path_ok(r, r->ecx) || !buffer_ok(r, r->edi, sizeof(statx_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_statx((int)r->ebx, (const char *)r->ecx, (int)r->edx, r->esi, (statx_t *)r->edi);
}
static uint32_t do_access(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return (uint32_t)sys_access((const char *)r->ebx, (int)r->ecx);
}
static uint32_t do_faccessat(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_faccessat((int)r->ebx, (const char *)r->ecx, (int)r->edx);
}
static uint32_t do_faccessat2(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_faccessat2((int)r->ebx, (const char *)r->ecx, (int)r->edx, (int)r->esi);
}
static uint32_t do_chmod(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return (uint32_t)sys_chmod((const char *)r->ebx, r->ecx);
}
static uint32_t do_fchmod(registers_t *r) { return (uint32_t)sys_fchmod((int)r->ebx, r->ecx); }
static uint32_t do_fchmodat(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_fchmodat((int)r->ebx, (const char *)r->ecx, r->edx);
}
static uint32_t do_chown(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return (uint32_t)sys_chown((const char *)r->ebx, r->ecx, r->edx);
}
static uint32_t do_fchown(registers_t *r) { return (uint32_t)sys_fchown((int)r->ebx, r->ecx, r->edx); }
static uint32_t do_lchown(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return (uint32_t)sys_lchown((const char *)r->ebx, r->ecx, r->edx);
}
static uint32_t do_fchownat(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_fchownat((int)r->ebx, (const char *)r->ecx, r->edx, r->esi, (int)r->edi);
}
static uint32_t do_umask(registers_t *r) { return sys_umask(r->ebx); }
static uint32_t do_truncate(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return (uint32_t)sys_truncate((const char *)r->ebx, r->ecx);
}
static uint32_t do_ftruncate(registers_t *r) { return (uint32_t)sys_ftruncate((int)r->ebx, r->ecx); }
static uint32_t do_fallocate(registers_t *r) {
    return (uint32_t)sys_fallocate((int)r->ebx, (int)r->ecx, r->edx, r->esi);
}
static uint32_t do_utime(registers_t *r) {
    if (!path_ok(r, r->ebx) || !optional_ok(r, r->ecx, sizeof(utimbuf_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_utime((const char *)r->ebx, (const utimbuf_t *)r->ecx);
}
static uint32_t do_utimes(registers_t *r) {
    if (!path_ok(r, r->ebx) || !optional_ok(r, r->ecx, 2 * sizeof(utimes_timeval_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_utimes((const char *)r->ebx, (const utimes_timeval_t *)r->ecx);
}
static uint32_t do_utimensat(registers_t *r) {
    if (!path_ok(r, r->ecx) || !optional_ok(r, r->edx, 2 * sizeof(utimensat_ts_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_utimensat((int)r->ebx, (const char *)r->ecx, (const utimensat_ts_t *)r->edx, (int)r->esi);
}

/* filesystem_management: directories */
static uint32_t do_rmdir(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;
    return (uint32_t)sys_rmdir((const char *)r->ebx);
}
static uint32_t do_mkdirat(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_mkdirat((int)r->ebx, (const char *)r->ecx, r->edx);
}
static uint32_t do_unlinkat(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_unlinkat((int)r->ebx, (const char *)r->ecx, (int)r->edx);
}
static uint32_t do_renameat(registers_t *r) {
    if (!path_ok(r, r->ecx) || !path_ok(r, r->esi)) return SYSCALL_ERROR;
    return (uint32_t)sys_renameat((int)r->ebx, (const char *)r->ecx, (int)r->edx, (const char *)r->esi);
}
static uint32_t do_renameat2(registers_t *r) {
    if (!path_ok(r, r->ecx) || !path_ok(r, r->esi)) return SYSCALL_ERROR;
    return (uint32_t)sys_renameat2((int)r->ebx, (const char *)r->ecx, (int)r->edx,
                                   (const char *)r->esi, (int)r->edi);
}
static uint32_t do_fchdir(registers_t *r) { return (uint32_t)sys_fchdir((int)r->ebx); }
static uint32_t do_getdents(registers_t *r) {
    if (!buffer_ok(r, r->ecx, r->edx)) return SYSCALL_ERROR;
    return (uint32_t)sys_getdents((int)r->ebx, (uint8_t *)r->ecx, r->edx);
}
static uint32_t do_getdents64(registers_t *r) {
    if (!buffer_ok(r, r->ecx, r->edx)) return SYSCALL_ERROR;
    return (uint32_t)sys_getdents64((int)r->ebx, (uint8_t *)r->ecx, r->edx);
}

/* filesystem_management: links */
static uint32_t do_link(registers_t *r) {
    if (!path_ok(r, r->ebx) || !path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_link((const char *)r->ebx, (const char *)r->ecx);
}
static uint32_t do_linkat(registers_t *r) {
    if (!path_ok(r, r->ecx) || !path_ok(r, r->esi)) return SYSCALL_ERROR;
    return (uint32_t)sys_linkat((int)r->ebx, (const char *)r->ecx, (int)r->edx,
                                (const char *)r->esi, (int)r->edi);
}
static uint32_t do_symlink(registers_t *r) {
    if (!path_ok(r, r->ebx) || !path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_symlink((const char *)r->ebx, (const char *)r->ecx);
}
static uint32_t do_symlinkat(registers_t *r) {
    if (!path_ok(r, r->ebx) || !path_ok(r, r->edx)) return SYSCALL_ERROR;
    return (uint32_t)sys_symlinkat((const char *)r->ebx, (int)r->ecx, (const char *)r->edx);
}
static uint32_t do_readlink(registers_t *r) {
    if (!path_ok(r, r->ebx) || !buffer_ok(r, r->ecx, r->edx)) return SYSCALL_ERROR;
    return (uint32_t)sys_readlink((const char *)r->ebx, (char *)r->ecx, r->edx);
}
static uint32_t do_readlinkat(registers_t *r) {
    if (!path_ok(r, r->ecx) || !buffer_ok(r, r->edx, r->esi)) return SYSCALL_ERROR;
    return (uint32_t)sys_readlinkat((int)r->ebx, (const char *)r->ecx, (char *)r->edx, r->esi);
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

/* process_management: identity, groups, sessions */
static uint32_t do_getppid(registers_t *r) { (void)r; return sys_getppid(); }
static uint32_t do_setpgid(registers_t *r) { return (uint32_t)sys_setpgid(r->ebx, r->ecx); }
static uint32_t do_getpgid(registers_t *r) { return (uint32_t)sys_getpgid(r->ebx); }
static uint32_t do_getpgrp(registers_t *r) { (void)r; return sys_getpgrp(); }
static uint32_t do_setsid(registers_t *r)  { (void)r; return (uint32_t)sys_setsid(); }
static uint32_t do_getsid(registers_t *r)  { return (uint32_t)sys_getsid(r->ebx); }
/* process_management: signals */
static uint32_t do_kill(registers_t *r)    { return (uint32_t)sys_kill((int32_t)r->ebx, (int)r->ecx); }
static uint32_t do_tkill(registers_t *r)   { return (uint32_t)sys_tkill(r->ebx, (int)r->ecx); }
static uint32_t do_tgkill(registers_t *r)  { return (uint32_t)sys_tgkill(r->ebx, r->ecx, (int)r->edx); }
static uint32_t do_pidfd_send_signal(registers_t *r) { return (uint32_t)sys_pidfd_send_signal((int)r->ebx, (int)r->ecx); }
/* process_management: wait */
static uint32_t do_wait(registers_t *r)    { (void)r; return (uint32_t)sys_wait(); }
static uint32_t do_waitpid(registers_t *r) { return (uint32_t)sys_waitpid(r->ebx); }
static uint32_t do_waitid(registers_t *r) {
    if (!optional_ok(r, r->edx, sizeof(waitid_info_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_waitid((int)r->ebx, r->ecx, (waitid_info_t *)r->edx, r->esi);
}
/* process_management: fork (-1: one address space), vfork + execve (real) */
static uint32_t do_fork(registers_t *r)     { (void)r; return (uint32_t)sys_fork(); }
static uint32_t do_vfork(registers_t *r)    { return (uint32_t)sys_vfork(r); }
static uint32_t do_execve(registers_t *r) {
    if (!path_ok(r, r->ebx)) return SYSCALL_ERROR;      // argv / envp: arg_limits checks them
    return (uint32_t)sys_execve(r, (const char *)r->ebx, (char *const *)r->ecx, (char *const *)r->edx);
}
static uint32_t do_execveat(registers_t *r) {
    if (!path_ok(r, r->ecx)) return SYSCALL_ERROR;
    return (uint32_t)sys_execveat(r, (int)r->ebx, (const char *)r->ecx, (char *const *)r->edx,
                                  (char *const *)r->esi, (int)r->edi);
}

/* process_management: signals with handlers, masks, queues, alarms */
static uint32_t sigset_bytes(uint32_t size) { return size == 8 ? 8 : 4; }
static uint32_t do_rt_sigaction(registers_t *r) {
    if (!optional_ok(r, r->ecx, sizeof(k_sigaction_t)) ||
        !optional_ok(r, r->edx, sizeof(k_sigaction_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_sigaction((int)r->ebx, (const k_sigaction_t *)r->ecx,
                                      (k_sigaction_t *)r->edx, r->esi);
}
static uint32_t do_rt_sigprocmask(registers_t *r) {
    uint32_t n = sigset_bytes(r->esi);
    if (!optional_ok(r, r->ecx, n) || !optional_ok(r, r->edx, n)) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_sigprocmask((int)r->ebx, (const uint32_t *)r->ecx, (uint32_t *)r->edx, r->esi);
}
static uint32_t do_rt_sigpending(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sigset_bytes(r->ecx))) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_sigpending((uint32_t *)r->ebx, r->ecx);
}
static uint32_t do_rt_sigsuspend(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sigset_bytes(r->ecx))) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_sigsuspend((const uint32_t *)r->ebx, r->ecx);
}
static uint32_t do_rt_sigtimedwait(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sigset_bytes(r->esi)) ||
        !optional_ok(r, r->ecx, sizeof(k_siginfo_t)) ||
        !optional_ok(r, r->edx, sizeof(sigwait_timeout_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_sigtimedwait((const uint32_t *)r->ebx, (k_siginfo_t *)r->ecx,
                                         (const sigwait_timeout_t *)r->edx, r->esi);
}
static uint32_t do_rt_sigqueueinfo(registers_t *r) {
    if (!buffer_ok(r, r->edx, sizeof(k_siginfo_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_sigqueueinfo(r->ebx, (int)r->ecx, (const k_siginfo_t *)r->edx);
}
static uint32_t do_rt_tgsigqueueinfo(registers_t *r) {
    if (!buffer_ok(r, r->esi, sizeof(k_siginfo_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_rt_tgsigqueueinfo(r->ebx, r->ecx, (int)r->edx, (const k_siginfo_t *)r->esi);
}
static uint32_t do_rt_sigreturn(registers_t *r) { return sys_rt_sigreturn(r); }
static uint32_t do_sigaltstack(registers_t *r) {
    if (!optional_ok(r, r->ebx, sizeof(k_stack_t)) ||
        !optional_ok(r, r->ecx, sizeof(k_stack_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_sigaltstack((const k_stack_t *)r->ebx, (k_stack_t *)r->ecx,
                                     from_user(r) ? r->useresp : 0);
}
static uint32_t do_pause(registers_t *r) { (void)r; return (uint32_t)sys_pause(); }
static uint32_t do_alarm(registers_t *r) { return sys_alarm(r->ebx); }
static uint32_t do_prctl(registers_t *r) {
    int option = (int)r->ebx;
    if (option == PR_SET_NAME && !path_ok(r, r->ecx)) return SYSCALL_ERROR;
    if ((option == PR_GET_NAME && !buffer_ok(r, r->ecx, PR_NAME_LEN)) ||
        ((option == PR_GET_PDEATHSIG || option == PR_GET_CHILD_SUBREAPER) &&
         !buffer_ok(r, r->ecx, sizeof(int)))) return SYSCALL_ERROR;
    return (uint32_t)sys_prctl(option, r->ecx);
}
/* process_management: wait with status and usage, pidfds */
static uint32_t do_wait4(registers_t *r) {
    if (!optional_ok(r, r->ecx, sizeof(int32_t)) ||
        !optional_ok(r, r->esi, sizeof(wait4_rusage_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_wait4((int32_t)r->ebx, (int32_t *)r->ecx, r->edx, (wait4_rusage_t *)r->esi);
}
static uint32_t do_getrusage(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(rusage_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_getrusage((int)r->ebx, (rusage_t *)r->ecx);
}
static uint32_t do_times(registers_t *r) {
    if (!optional_ok(r, r->ebx, sizeof(tms_t))) return SYSCALL_ERROR;
    return sys_times((tms_t *)r->ebx);
}
static uint32_t do_pidfd_open(registers_t *r)  { return (uint32_t)sys_pidfd_open(r->ebx, r->ecx); }
static uint32_t do_pidfd_getfd(registers_t *r) { return (uint32_t)sys_pidfd_getfd((int)r->ebx, (int)r->ecx, r->edx); }
/* process_management: sleeping and interval timers */
static uint32_t do_nanosleep(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sizeof(nanosleep_timespec_t)) ||
        !optional_ok(r, r->ecx, sizeof(nanosleep_timespec_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_nanosleep((const nanosleep_timespec_t *)r->ebx, (nanosleep_timespec_t *)r->ecx);
}
static uint32_t do_clock_nanosleep(registers_t *r) {
    if (!buffer_ok(r, r->edx, sizeof(clock_timespec_t)) ||
        !optional_ok(r, r->esi, sizeof(clock_timespec_t))) return CNS_EINVAL;
    return (uint32_t)sys_clock_nanosleep((int)r->ebx, (int)r->ecx, (const clock_timespec_t *)r->edx,
                                         (clock_timespec_t *)r->esi);
}
static uint32_t do_setitimer(registers_t *r) {
    if (!optional_ok(r, r->ecx, sizeof(setitimer_val_t)) ||
        !optional_ok(r, r->edx, sizeof(setitimer_val_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_setitimer((int)r->ebx, (const setitimer_val_t *)r->ecx, (setitimer_val_t *)r->edx);
}
/* process_management: sandboxes */
static uint32_t do_seccomp(registers_t *r) {
    uint32_t op = r->ebx;
    uint32_t size = op == SECCOMP_SET_MODE_FILTER ? sizeof(seccomp_list_t) : sizeof(uint32_t);
    if (!optional_ok(r, r->edx, size)) return SYSCALL_ERROR;
    return (uint32_t)sys_seccomp(op, r->ecx, (void *)r->edx);
}
static uint32_t do_unshare(registers_t *r) { return (uint32_t)sys_unshare(r->ebx); }
static uint32_t do_setns(registers_t *r)   { return (uint32_t)sys_setns((int)r->ebx, r->ecx); }
/* process_management: debugging */
static uint32_t do_ptrace(registers_t *r) {
    int req = (int)r->ebx;
    if ((req == PTRACE_PEEKDATA && !buffer_ok(r, r->esi, 4)) ||
        (req == PTRACE_GETREGS && !buffer_ok(r, r->esi, sizeof(ptrace_regs_t)))) return SYSCALL_ERROR;
    return (uint32_t)sys_ptrace(req, r->ecx, r->edx, r->esi);
}
// The local side of process_vm_readv / writev: the array and every piece
// must be the caller's own memory (the remote side is vm_access_check's).
static int local_iov_ok(registers_t *r, uint32_t iov, uint32_t count) {
    if (count > VM_IOV_MAX || !buffer_ok(r, iov, count * 8))
        return 0;
    for (uint32_t i = 0; i < count; i++) {
        const uint32_t *piece = (const uint32_t *)(iov + i * 8);
        if (piece[1] && !buffer_ok(r, piece[0], piece[1]))
            return 0;
    }
    return 1;
}
static uint32_t do_process_vm_readv(registers_t *r) {
    if (!local_iov_ok(r, r->ecx, r->edx) || r->edi > VM_IOV_MAX ||
        !buffer_ok(r, r->esi, r->edi * 8)) return SYSCALL_ERROR;
    return (uint32_t)sys_process_vm_readv(r->ebx, (const vm_readv_iovec_t *)r->ecx, r->edx,
                                          (const vm_readv_iovec_t *)r->esi, r->edi, r->ebp);
}
static uint32_t do_process_vm_writev(registers_t *r) {
    if (!local_iov_ok(r, r->ecx, r->edx) || r->edi > VM_IOV_MAX ||
        !buffer_ok(r, r->esi, r->edi * 8)) return SYSCALL_ERROR;
    return (uint32_t)sys_process_vm_writev(r->ebx, (const vm_writev_iovec_t *)r->ecx, r->edx,
                                           (const vm_writev_iovec_t *)r->esi, r->edi, r->ebp);
}
static uint32_t do_kcmp(registers_t *r) {
    return (uint32_t)sys_kcmp(r->ebx, r->ecx, (int)r->edx, r->esi, r->edi);
}

static uint32_t do_getitimer(registers_t *r) {
    if (!buffer_ok(r, r->ecx, sizeof(getitimer_val_t))) return SYSCALL_ERROR;
    return (uint32_t)sys_getitimer((int)r->ebx, (getitimer_val_t *)r->ecx);
}

/* network_management: sockets. Errors are -errno (Linux's numbers), and
 * a bad pointer is -EFAULT. An address is copied into the kernel before
 * it is used, so the program cannot change it half-way. */

#define NET_FAULT ((uint32_t)-NET_EFAULT)

// Copy a sockaddr from the caller (len bytes, at most sizeof(sockaddr_t)).
static int copy_addr(registers_t *r, uint32_t addr, uint32_t len, sockaddr_t *out) {
    if (len > sizeof(sockaddr_t) || !buffer_ok(r, addr, len))
        return 0;
    memcpy(out, (const void *)addr, len);
    return 1;
}

// An (addr, &len) pair to be filled in: both optional, both checked.
static int out_addr_ok(registers_t *r, uint32_t addr, uint32_t len_ptr) {
    if (!addr)
        return 1;
    return buffer_ok(r, len_ptr, sizeof(uint32_t)) && len_ptr &&
           buffer_ok(r, addr, *(uint32_t *)len_ptr);
}

static uint32_t do_socket(registers_t *r) {
    return (uint32_t)sys_socket((int)r->ebx, (int)r->ecx, (int)r->edx);
}
static uint32_t do_bind(registers_t *r) {
    sockaddr_t a;
    if (!copy_addr(r, r->ecx, r->edx, &a)) return NET_FAULT;
    return (uint32_t)sys_bind((int)r->ebx, &a, r->edx);
}
static uint32_t do_connect(registers_t *r) {
    sockaddr_t a;
    if (!copy_addr(r, r->ecx, r->edx, &a)) return NET_FAULT;
    return (uint32_t)sys_connect((int)r->ebx, &a, r->edx);
}
static uint32_t do_listen(registers_t *r) {
    return (uint32_t)sys_listen((int)r->ebx, (int)r->ecx);
}
static uint32_t do_accept(registers_t *r) {
    if (!out_addr_ok(r, r->ecx, r->edx)) return NET_FAULT;
    return (uint32_t)sys_accept((int)r->ebx, (sockaddr_t *)r->ecx, (uint32_t *)r->edx);
}
static uint32_t do_accept4(registers_t *r) {
    if (!out_addr_ok(r, r->ecx, r->edx)) return NET_FAULT;
    return (uint32_t)sys_accept4((int)r->ebx, (sockaddr_t *)r->ecx, (uint32_t *)r->edx, (int)r->esi);
}
static uint32_t do_getsockname(registers_t *r) {
    if (!r->ecx || !out_addr_ok(r, r->ecx, r->edx)) return NET_FAULT;
    return (uint32_t)sys_getsockname((int)r->ebx, (sockaddr_t *)r->ecx, (uint32_t *)r->edx);
}
static uint32_t do_getpeername(registers_t *r) {
    if (!r->ecx || !out_addr_ok(r, r->ecx, r->edx)) return NET_FAULT;
    return (uint32_t)sys_getpeername((int)r->ebx, (sockaddr_t *)r->ecx, (uint32_t *)r->edx);
}
static uint32_t do_sendto(registers_t *r) {
    sockaddr_t a;
    if (!buffer_ok(r, r->ecx, r->edx)) return NET_FAULT;
    if (r->edi && !copy_addr(r, r->edi, r->ebp, &a)) return NET_FAULT;
    return (uint32_t)sys_sendto((int)r->ebx, (const void *)r->ecx, r->edx, (int)r->esi,
                                r->edi ? &a : 0, r->ebp);
}
static uint32_t do_recvfrom(registers_t *r) {
    if (!buffer_ok(r, r->ecx, r->edx) || !out_addr_ok(r, r->edi, r->ebp)) return NET_FAULT;
    return (uint32_t)sys_recvfrom((int)r->ebx, (void *)r->ecx, r->edx, (int)r->esi,
                                  (sockaddr_t *)r->edi, (uint32_t *)r->ebp);
}
static uint32_t do_shutdown(registers_t *r) {
    return (uint32_t)sys_shutdown((int)r->ebx, (int)r->ecx);
}
static uint32_t do_setsockopt(registers_t *r) {
    if (!buffer_ok(r, r->esi, r->edi)) return NET_FAULT;
    return (uint32_t)sys_setsockopt((int)r->ebx, (int)r->ecx, (int)r->edx, (const void *)r->esi, r->edi);
}
static uint32_t do_getsockopt(registers_t *r) {
    if (!r->esi || !out_addr_ok(r, r->esi, r->edi)) return NET_FAULT;
    return (uint32_t)sys_getsockopt((int)r->ebx, (int)r->ecx, (int)r->edx, (void *)r->esi, (uint32_t *)r->edi);
}
static uint32_t do_uname(registers_t *r) {
    if (!buffer_ok(r, r->ebx, sizeof(utsname_t))) return NET_FAULT;
    return (uint32_t)sys_uname((utsname_t *)r->ebx);
}
static uint32_t do_sethostname(registers_t *r) {
    if (!buffer_ok(r, r->ebx, r->ecx)) return NET_FAULT;
    return (uint32_t)sys_sethostname((const char *)r->ebx, r->ecx);
}
static uint32_t do_setdomainname(registers_t *r) {
    if (!buffer_ok(r, r->ebx, r->ecx)) return NET_FAULT;
    return (uint32_t)sys_setdomainname((const char *)r->ebx, r->ecx);
}
static uint32_t do_gethostname(registers_t *r) {
    if (!buffer_ok(r, r->ebx, r->ecx)) return NET_FAULT;
    return (uint32_t)sys_gethostname((char *)r->ebx, r->ecx);
}

static uint32_t do_socketpair(registers_t *r) {
    if (!buffer_ok(r, r->esi, 2 * sizeof(int))) return NET_FAULT;
    return (uint32_t)sys_socketpair((int)r->ebx, (int)r->ecx, (int)r->edx, (int *)r->esi);
}

// A msghdr and everything it points at: the iovec, each buffer, the name.
static int msg_ok(registers_t *r, uint32_t msg) {
    if (!buffer_ok(r, msg, sizeof(msghdr_t)))
        return 0;
    const msghdr_t *m = (const msghdr_t *)msg;
    if (m->iovlen > MSG_IOV_MAX || !buffer_ok(r, (uint32_t)m->iov, m->iovlen * sizeof(iovec_t)))
        return 0;
    for (uint32_t i = 0; i < m->iovlen; i++)
        if (!buffer_ok(r, (uint32_t)m->iov[i].base, m->iov[i].len))
            return 0;
    return !m->name || (m->namelen <= sizeof(sockaddr_t) && buffer_ok(r, (uint32_t)m->name, m->namelen));
}

// An array of mmsghdr: the array, then each message.
static int mmsg_ok(registers_t *r, uint32_t msgs, uint32_t count) {
    if (count > MMSG_MAX || !buffer_ok(r, msgs, count * sizeof(mmsghdr_t)))
        return 0;
    for (uint32_t i = 0; i < count; i++)
        if (!msg_ok(r, (uint32_t)&((mmsghdr_t *)msgs)[i].hdr))
            return 0;
    return 1;
}

static uint32_t do_sendmsg(registers_t *r) {
    if (!msg_ok(r, r->ecx)) return NET_FAULT;
    return (uint32_t)sys_sendmsg((int)r->ebx, (const msghdr_t *)r->ecx, (int)r->edx);
}
static uint32_t do_recvmsg(registers_t *r) {
    if (!msg_ok(r, r->ecx)) return NET_FAULT;
    return (uint32_t)sys_recvmsg((int)r->ebx, (msghdr_t *)r->ecx, (int)r->edx);
}
static uint32_t do_sendmmsg(registers_t *r) {
    if (!mmsg_ok(r, r->ecx, r->edx)) return NET_FAULT;
    return (uint32_t)sys_sendmmsg((int)r->ebx, (mmsghdr_t *)r->ecx, r->edx, (int)r->esi);
}
static uint32_t do_recvmmsg(registers_t *r) {
    if (!mmsg_ok(r, r->ecx, r->edx)) return NET_FAULT;
    return (uint32_t)sys_recvmmsg((int)r->ebx, (mmsghdr_t *)r->ecx, r->edx, (int)r->esi);
}

// select's three sets and the timeout: each may be 0.
static int sets_ok(registers_t *r) {
    return optional_ok(r, r->ecx, sizeof(select_set_t)) && optional_ok(r, r->edx, sizeof(select_set_t)) &&
           optional_ok(r, r->esi, sizeof(select_set_t));
}
static uint32_t do_select(registers_t *r) {
    if (!sets_ok(r) || !optional_ok(r, r->edi, sizeof(select_timeval_t))) return NET_FAULT;
    return (uint32_t)sys_select((int)r->ebx, (select_set_t *)r->ecx, (select_set_t *)r->edx,
                                (select_set_t *)r->esi, (const select_timeval_t *)r->edi);
}
static uint32_t do_pselect6(registers_t *r) {
    if (!sets_ok(r) || !optional_ok(r, r->edi, sizeof(pselect_timespec_t)) ||
        !optional_ok(r, r->ebp, sizeof(pselect_sigdata_t))) return NET_FAULT;
    const pselect_sigdata_t *sig = (const pselect_sigdata_t *)r->ebp;
    if (sig && !optional_ok(r, (uint32_t)sig->mask, sizeof(uint32_t))) return NET_FAULT;
    return (uint32_t)sys_pselect6((int)r->ebx, (select_set_t *)r->ecx, (select_set_t *)r->edx,
                                  (select_set_t *)r->esi, (const pselect_timespec_t *)r->edi, sig);
}
static uint32_t do_poll(registers_t *r) {
    if (r->ecx > POLL_MAX) return (uint32_t)-NET_EINVAL;
    if (!buffer_ok(r, r->ebx, r->ecx * sizeof(pollfd_t))) return NET_FAULT;
    return (uint32_t)sys_poll((pollfd_t *)r->ebx, r->ecx, (int)r->edx);
}
static uint32_t do_ppoll(registers_t *r) {
    if (r->ecx > POLL_MAX) return (uint32_t)-NET_EINVAL;
    if (!buffer_ok(r, r->ebx, r->ecx * sizeof(pollfd_t)) || !optional_ok(r, r->edx, sizeof(ppoll_timespec_t)) ||
        !optional_ok(r, r->esi, sizeof(uint32_t))) return NET_FAULT;
    return (uint32_t)sys_ppoll((pollfd_t *)r->ebx, r->ecx, (const ppoll_timespec_t *)r->edx,
                               (const uint32_t *)r->esi, r->edi);
}
static uint32_t do_epoll_create(registers_t *r)  { return (uint32_t)sys_epoll_create((int)r->ebx); }
static uint32_t do_epoll_create1(registers_t *r) { return (uint32_t)sys_epoll_create1((int)r->ebx); }
static uint32_t do_epoll_ctl(registers_t *r) {
    if (!optional_ok(r, r->esi, sizeof(epoll_event_t))) return NET_FAULT;
    return (uint32_t)sys_epoll_ctl((int)r->ebx, (int)r->ecx, (int)r->edx, (const epoll_event_t *)r->esi);
}
static uint32_t do_epoll_wait(registers_t *r) {
    if ((int)r->edx <= 0 || r->edx > EPOLL_INTERESTS) return (uint32_t)-NET_EINVAL;
    if (!buffer_ok(r, r->ecx, r->edx * sizeof(epoll_event_t))) return NET_FAULT;
    return (uint32_t)sys_epoll_wait((int)r->ebx, (epoll_event_t *)r->ecx, (int)r->edx, (int)r->esi);
}
static uint32_t do_epoll_pwait(registers_t *r) {
    if ((int)r->edx <= 0 || r->edx > EPOLL_INTERESTS) return (uint32_t)-NET_EINVAL;
    if (!buffer_ok(r, r->ecx, r->edx * sizeof(epoll_event_t)) || !optional_ok(r, r->edi, sizeof(uint32_t)))
        return NET_FAULT;
    return (uint32_t)sys_epoll_pwait((int)r->ebx, (epoll_event_t *)r->ecx, (int)r->edx, (int)r->esi,
                                     (const uint32_t *)r->edi, r->ebp);
}
static uint32_t do_ioctl(registers_t *r) {
    // How big 'arg' is depends on the command: an int, or an ifreq.
    uint32_t size = (r->ecx == FIONREAD || r->ecx == FIONBIO) ? sizeof(int) : sizeof(ifreq_t);
    if (!buffer_ok(r, r->edx, size)) return NET_FAULT;
    return (uint32_t)sys_ioctl((int)r->ebx, r->ecx, (void *)r->edx);
}
static uint32_t do_sendfile(registers_t *r) {
    if (!optional_ok(r, r->edx, sizeof(uint32_t))) return NET_FAULT;
    return (uint32_t)sys_sendfile((int)r->ebx, (int)r->ecx, (uint32_t *)r->edx, r->esi);
}

// socketcall(call, args): put the array's values into the registers the
// direct syscall reads, and run its wrapper (which checks the pointers).
static uint32_t do_socketcall(registers_t *r) {
    int count = socketcall_arg_count((int)r->ebx);
    if (!count)
        return (uint32_t)-NET_EINVAL;
    if (!buffer_ok(r, r->ecx, count * sizeof(uint32_t))) return NET_FAULT;
    const uint32_t *a = (const uint32_t *)r->ecx;
    registers_t inner = *r;                 // same ring: same checks
    inner.ebx = a[0];
    inner.ecx = count > 1 ? a[1] : 0;
    inner.edx = count > 2 ? a[2] : 0;
    inner.esi = count > 3 ? a[3] : 0;
    inner.edi = count > 4 ? a[4] : 0;
    inner.ebp = count > 5 ? a[5] : 0;
    switch (r->ebx) {
    case SOCKETCALL_SOCKET:      return do_socket(&inner);
    case SOCKETCALL_BIND:        return do_bind(&inner);
    case SOCKETCALL_CONNECT:     return do_connect(&inner);
    case SOCKETCALL_LISTEN:      return do_listen(&inner);
    case SOCKETCALL_ACCEPT:      return do_accept(&inner);
    case SOCKETCALL_GETSOCKNAME: return do_getsockname(&inner);
    case SOCKETCALL_GETPEERNAME: return do_getpeername(&inner);
    case SOCKETCALL_SEND:        inner.edi = inner.ebp = 0; return do_sendto(&inner);   // send = sendto, no address
    case SOCKETCALL_RECV:        inner.edi = inner.ebp = 0; return do_recvfrom(&inner);
    case SOCKETCALL_SENDTO:      return do_sendto(&inner);
    case SOCKETCALL_RECVFROM:    return do_recvfrom(&inner);
    case SOCKETCALL_SHUTDOWN:    return do_shutdown(&inner);
    case SOCKETCALL_SETSOCKOPT:  return do_setsockopt(&inner);
    case SOCKETCALL_GETSOCKOPT:  return do_getsockopt(&inner);
    case SOCKETCALL_ACCEPT4:     return do_accept4(&inner);
    case SOCKETCALL_SOCKETPAIR:  return do_socketpair(&inner);
    case SOCKETCALL_SENDMSG:     return do_sendmsg(&inner);
    case SOCKETCALL_RECVMSG:     return do_recvmsg(&inner);
    case SOCKETCALL_SENDMMSG:    return do_sendmmsg(&inner);
    case SOCKETCALL_RECVMMSG:    return do_recvmmsg(&inner);
    default:                     return (uint32_t)-NET_EINVAL;
    }
}

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
    /* filesystem_management: links */
    { SYS_LINK,              "link",               do_link },
    { SYS_LINKAT,            "linkat",             do_linkat },
    { SYS_SYMLINK,           "symlink",            do_symlink },
    { SYS_SYMLINKAT,         "symlinkat",          do_symlinkat },
    { SYS_READLINK,          "readlink",           do_readlink },
    { SYS_READLINKAT,        "readlinkat",         do_readlinkat },
    /* filesystem_management: directories */
    { SYS_RMDIR,             "rmdir",              do_rmdir },
    { SYS_MKDIRAT,           "mkdirat",            do_mkdirat },
    { SYS_UNLINKAT,          "unlinkat",           do_unlinkat },
    { SYS_RENAMEAT,          "renameat",           do_renameat },
    { SYS_RENAMEAT2,         "renameat2",          do_renameat2 },
    { SYS_FCHDIR,            "fchdir",             do_fchdir },
    { SYS_GETDENTS,          "getdents",           do_getdents },
    { SYS_GETDENTS64,        "getdents64",         do_getdents64 },
    /* filesystem_management: metadata */
    { SYS_STAT,              "stat",               do_stat },
    { SYS_LSTAT,             "lstat",              do_lstat },
    { SYS_FSTAT,             "fstat",              do_fstat },
    { SYS_FSTATAT,           "fstatat",            do_fstatat },
    { SYS_STATX,             "statx",              do_statx },
    { SYS_ACCESS,            "access",             do_access },
    { SYS_FACCESSAT,         "faccessat",          do_faccessat },
    { SYS_FACCESSAT2,        "faccessat2",         do_faccessat2 },
    { SYS_CHMOD,             "chmod",              do_chmod },
    { SYS_FCHMOD,            "fchmod",             do_fchmod },
    { SYS_FCHMODAT,          "fchmodat",           do_fchmodat },
    { SYS_CHOWN,             "chown",              do_chown },
    { SYS_FCHOWN,            "fchown",             do_fchown },
    { SYS_LCHOWN,            "lchown",             do_lchown },
    { SYS_FCHOWNAT,          "fchownat",           do_fchownat },
    { SYS_UMASK,             "umask",              do_umask },
    { SYS_TRUNCATE,          "truncate",           do_truncate },
    { SYS_FTRUNCATE,         "ftruncate",          do_ftruncate },
    { SYS_FALLOCATE,         "fallocate",          do_fallocate },
    { SYS_UTIME,             "utime",              do_utime },
    { SYS_UTIMES,            "utimes",             do_utimes },
    { SYS_UTIMENSAT,         "utimensat",          do_utimensat },
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
    /* process_management: identity, groups, sessions */
    { SYS_GETPPID,            "getppid",            do_getppid            },
    { SYS_SETPGID,            "setpgid",            do_setpgid            },
    { SYS_GETPGID,            "getpgid",            do_getpgid            },
    { SYS_GETPGRP,            "getpgrp",            do_getpgrp            },
    { SYS_SETSID,             "setsid",             do_setsid             },
    { SYS_GETSID,             "getsid",             do_getsid             },
    /* process_management: signals */
    { SYS_KILL,               "kill",               do_kill               },
    { SYS_TKILL,              "tkill",              do_tkill              },
    { SYS_TGKILL,             "tgkill",             do_tgkill             },
    { SYS_PIDFD_SEND_SIGNAL,  "pidfd_send_signal",  do_pidfd_send_signal  },
    /* process_management: wait */
    { SYS_WAIT,               "wait",               do_wait               },
    { SYS_WAITPID,            "waitpid",            do_waitpid            },
    { SYS_WAITID,             "waitid",             do_waitid             },
    /* process_management: fork (-1), vfork, execve, execveat */
    { SYS_FORK,               "fork",               do_fork               },
    { SYS_VFORK,              "vfork",              do_vfork              },
    { SYS_EXECVE,             "execve",             do_execve             },
    { SYS_EXECVEAT,           "execveat",           do_execveat           },
    /* process_management: signals with handlers */
    { SYS_RT_SIGACTION,       "rt_sigaction",       do_rt_sigaction       },
    { SYS_RT_SIGPROCMASK,     "rt_sigprocmask",     do_rt_sigprocmask     },
    { SYS_RT_SIGPENDING,      "rt_sigpending",      do_rt_sigpending      },
    { SYS_RT_SIGSUSPEND,      "rt_sigsuspend",      do_rt_sigsuspend      },
    { SYS_RT_SIGTIMEDWAIT,    "rt_sigtimedwait",    do_rt_sigtimedwait    },
    { SYS_RT_SIGQUEUEINFO,    "rt_sigqueueinfo",    do_rt_sigqueueinfo    },
    { SYS_RT_TGSIGQUEUEINFO,  "rt_tgsigqueueinfo",  do_rt_tgsigqueueinfo  },
    { SYS_RT_SIGRETURN,       "rt_sigreturn",       do_rt_sigreturn       },
    { SYS_SIGALTSTACK,        "sigaltstack",        do_sigaltstack        },
    { SYS_PAUSE,              "pause",              do_pause              },
    { SYS_ALARM,              "alarm",              do_alarm              },
    { SYS_PRCTL,              "prctl",              do_prctl              },
    { SYS_WAIT4,              "wait4",              do_wait4              },
    { SYS_GETRUSAGE,          "getrusage",          do_getrusage          },
    { SYS_TIMES,              "times",              do_times              },
    { SYS_PIDFD_OPEN,         "pidfd_open",         do_pidfd_open         },
    { SYS_PIDFD_GETFD,        "pidfd_getfd",        do_pidfd_getfd        },
    { SYS_NANOSLEEP,          "nanosleep",          do_nanosleep          },
    { SYS_CLOCK_NANOSLEEP,    "clock_nanosleep",    do_clock_nanosleep    },
    { SYS_SETITIMER,          "setitimer",          do_setitimer          },
    { SYS_GETITIMER,          "getitimer",          do_getitimer          },
    { SYS_SECCOMP,            "seccomp",            do_seccomp            },
    { SYS_UNSHARE,            "unshare",            do_unshare            },
    { SYS_SETNS,              "setns",              do_setns              },
    { SYS_PTRACE,             "ptrace",             do_ptrace             },
    { SYS_PROCESS_VM_READV,   "process_vm_readv",   do_process_vm_readv   },
    { SYS_PROCESS_VM_WRITEV,  "process_vm_writev",  do_process_vm_writev  },
    { SYS_KCMP,               "kcmp",               do_kcmp               },
    /* network_management: sockets and names */
    { SYS_SOCKET,             "socket",             do_socket             },
    { SYS_BIND,               "bind",               do_bind               },
    { SYS_CONNECT,            "connect",            do_connect            },
    { SYS_LISTEN,             "listen",             do_listen             },
    { SYS_ACCEPT,             "accept",             do_accept             },
    { SYS_ACCEPT4,            "accept4",            do_accept4            },
    { SYS_GETSOCKNAME,        "getsockname",        do_getsockname        },
    { SYS_GETPEERNAME,        "getpeername",        do_getpeername        },
    { SYS_SENDTO,             "sendto",             do_sendto             },
    { SYS_RECVFROM,           "recvfrom",           do_recvfrom           },
    { SYS_SHUTDOWN,           "shutdown",           do_shutdown           },
    { SYS_SETSOCKOPT,         "setsockopt",         do_setsockopt         },
    { SYS_GETSOCKOPT,         "getsockopt",         do_getsockopt         },
    { SYS_SOCKETCALL,         "socketcall",         do_socketcall         },
    { SYS_UNAME,              "uname",              do_uname              },
    { SYS_SETHOSTNAME,        "sethostname",        do_sethostname        },
    { SYS_SETDOMAINNAME,      "setdomainname",      do_setdomainname      },
    { SYS_GETHOSTNAME,        "gethostname",        do_gethostname        },
    { SYS_SOCKETPAIR,         "socketpair",         do_socketpair         },
    { SYS_SENDMSG,            "sendmsg",            do_sendmsg            },
    { SYS_RECVMSG,            "recvmsg",            do_recvmsg            },
    { SYS_SENDMMSG,           "sendmmsg",           do_sendmmsg           },
    { SYS_RECVMMSG,           "recvmmsg",           do_recvmmsg           },
    { SYS_SELECT,             "select",             do_select             },
    { SYS_PSELECT6,           "pselect6",           do_pselect6           },
    { SYS_POLL,               "poll",               do_poll               },
    { SYS_PPOLL,              "ppoll",              do_ppoll              },
    { SYS_EPOLL_CREATE,       "epoll_create",       do_epoll_create       },
    { SYS_EPOLL_CREATE1,      "epoll_create1",      do_epoll_create1      },
    { SYS_EPOLL_CTL,          "epoll_ctl",          do_epoll_ctl          },
    { SYS_EPOLL_WAIT,         "epoll_wait",         do_epoll_wait         },
    { SYS_EPOLL_PWAIT,        "epoll_pwait",        do_epoll_pwait        },
    { SYS_IOCTL,              "ioctl",              do_ioctl              },
    { SYS_SENDFILE,           "sendfile",           do_sendfile           },
};
#define TABLE_SIZE (sizeof(table) / sizeof(table[0]))

// Show syscalls that come from user mode, so you can watch ring 3 work.
static void trace(const char *name) {
    uint8_t color = vga_get_color();        // put back whatever was in use
    ui_color(UI_DIM);
    ui_printf("[syscall] PID %d (ring 3) -> %s\n", sys_getpid(), name);
    vga_set_color_byte(color);
}

// DEFENCE seccomp_filter: a sandboxed program only gets the syscalls it
// kept. 1 = go on; 0 = refused (the call fails, or the program ends).
static int sandbox_allows(registers_t *regs) {
    process_t *self = scheduler_get_current_process();
    if (!self || !self->seccomp_mode || def_seccomp_allowed(self, regs->eax))
        return 1;
    uint32_t action = def_seccomp_action(self);
    if (action == SECCOMP_RET_KILL_PROCESS || action == SECCOMP_RET_KILL_THREAD) {
        trace("seccomp: SIGSYS");
        process_exit_with(SIGSYS | 0x80);       // as Linux: SIGSYS, core dumped
    }
    regs->eax = SYSCALL_ERROR;                  // SECCOMP_RET_ERRNO: just fails
    return 0;
}

// `strace`: one line per call of a traced process, with its first three
// arguments and its answer (a call that never returns prints "= ?").
static void strace_line(const char *name, const registers_t *in, uint32_t result, int returned) {
    uint8_t color = vga_get_color();
    ui_color(UI_ACCENT);
    ui_printf("[strace] %d %s(0x%x, 0x%x, 0x%x)", sys_getpid(), name, in->ebx, in->ecx, in->edx);
    if (returned)
        ui_printf(" = %d\n", (int32_t)result);
    else
        ui_printf(" = ?\n");
    vga_set_color_byte(color);
}

static void syscall_handler(registers_t *regs) {
    if (!sandbox_allows(regs))
        return;
    process_t *self = scheduler_get_current_process();
    int traced = self && self->strace;
    for (unsigned i = 0; i < TABLE_SIZE; i++) {
        if (table[i].number != regs->eax)
            continue;
        registers_t in = *regs;             // the arguments, before the call changes them
        if (traced && (in.eax == SYS_EXIT || in.eax == SYS_EXIT_GROUP))
            strace_line(table[i].name, &in, 0, 0);
        else if (from_user(regs) && !traced)
            trace(table[i].name);
        regs->eax = table[i].run(regs);
        if (traced)
            strace_line(table[i].name, &in, regs->eax, 1);
        return;
    }
    if (from_user(regs))
        trace("unknown");
    regs->eax = SYSCALL_ERROR;          // no such syscall
}

void init_syscalls(void) {
    register_interrupt_handler(SYSCALL_VECTOR, syscall_handler);
}
