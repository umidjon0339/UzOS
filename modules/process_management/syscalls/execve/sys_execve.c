/*
 * sys_execve.c - execve(path, argv, envp): the caller becomes another program.
 *
 * Same process (pid, parent, user, open files), new program:
 *
 *   1. check and COPY argv / envp into the kernel (arg_limits; envp
 *      entries also env_sanitize): the old program's memory may be
 *      overwritten by the new one
 *   2. open the ELF file and check it (elf_validate), and that it keeps
 *      clear of the stacks (guard_gap) - up to here a failure returns
 *      -1 and the old program carries on
 *   3. load its segments (mapped as user pages)
 *   4. build the new user stack, as the System V ABI wants it:
 *        esp -> argc | argv[0..n-1] | NULL | envp[0..m-1] | NULL | strings
 *   5. close O_CLOEXEC files and pidfds (close_on_exec); handlers back
 *      to SIG_DFL (an ignored signal stays ignored); a vfork() parent
 *      waiting for us may go on
 *   6. start it in RING 3 at e_entry - always: a new program never gets
 *      more privilege than a user program has (user_stack checks ESP)
 *
 * UzOS has one address space: the new program's pages are added to it,
 * the old ones are not given back (there is no "its own" memory to
 * free). The stack comes from the process's own 64 KiB area, so two
 * programs never share a stack.
 *
 * Uses: scheduler_get_current_process (core/scheduler), vfs_lookup /
 *       vfs_read / vfs_cwd (filesystem_management), perm_check
 *       (user_management), pmm_alloc_block / vmm_map_page / vmm_walk
 *       (memory_management), kmalloc / kfree, integrity_verify
 *       (security_management), switch_to_user_mode (arch/x86/usermode),
 *       and the defences arg_limits, env_sanitize, elf_validate,
 *       guard_gap, close_on_exec, user_stack.
 */
#include "modules/process_management/syscalls/execve/sys_execve.h"                                        // this file
#include "modules/process_management/core/scheduler/scheduler.h"                                          // scheduler_get_current_process
#include "modules/process_management/core/signal/signal.h"                                                // SIG_IGN, SIGNAL_COUNT
#include "modules/process_management/core/elf/elf.h"                                                      // Elf32_Ehdr, Elf32_Phdr, PT_LOAD
#include "modules/filesystem_management/core/vfs/vfs.h"                                                   // vfs_lookup, vfs_read, vfs_cwd
#include "modules/user_management/core/perm/perm.h"                                                       // perm_check, PERM_READ
#include "modules/memory_management/core/pmm/pmm.h"                                                       // pmm_alloc_block
#include "modules/memory_management/core/vmm/vmm.h"                                                       // vmm_map_page, vmm_walk
#include "modules/memory_management/core/kheap/kheap.h"                                                   // kmalloc, kfree
#include "modules/memory_management/core/layout/memory_layout.h"                                          // EXEC_STACKS_START
#include "modules/security_management/core/integrity/integrity.h"                                         // integrity_verify
#include "arch/x86/usermode/usermode.h"                                                                   // switch_to_user_mode
#include "lib/string/string.h"                                                                            // memset, memcpy, strlen, strncpy
#include "modules/process_management/security/argv_overflow/defence/arg_limits/def_arg_limits.h"          // def_args_ok
#include "modules/process_management/security/env_injection/defence/env_sanitize/def_env_sanitize.h"      // def_env_keep
#include "modules/process_management/security/malicious_elf/defence/elf_validate/def_elf_validate.h"      // def_elf_check_header, def_elf_check_segments
#include "modules/process_management/security/fd_leak_on_exec/defence/close_on_exec/def_close_on_exec.h"  // def_close_on_exec
#include "modules/process_management/security/kernel_stack_to_user/defence/user_stack/def_user_stack.h"   // def_user_stack_ok
#include "modules/process_management/security/stack_clash/defence/guard_gap/def_guard_gap.h"              // def_segments_clear_of_stacks, def_guard_gap_ok

#define AREA_SIZE   0x10000             // each slot's area in EXEC_STACKS
#define FLAGS_USER  0x202               // IF on, bit 1

// The copied arguments: strings back to back, and where each one starts.
typedef struct {
    char    *text;
    uint32_t used;
    uint32_t argc, envc;
    uint32_t start[ARG_MAX_STRINGS];
} args_t;

static void copy_list(args_t *a, char *const list[], int is_env) {
    for (int i = 0; list && list[i]; i++) {
        // DEFENCE env_sanitize: a dropped entry never reaches the program.
        if (is_env && !def_env_keep(list[i]))
            continue;
        uint32_t n = strlen(list[i]) + 1;
        a->start[a->argc + a->envc] = a->used;
        memcpy(a->text + a->used, list[i], n);
        a->used += n;
        if (is_env) a->envc++; else a->argc++;
    }
}

// Map the pages a segment covers (reusing ones already mapped), zero it,
// copy its bytes from the file. (The same as core/elf's, copied here.)
static int load_segment(vfs_node_t *file, const Elf32_Phdr *ph) {
    uint32_t first = ph->p_vaddr & ~(PAGE_SIZE - 1);
    uint32_t end   = (ph->p_vaddr + ph->p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    for (uint32_t page = first; page < end; page += PAGE_SIZE) {
        vmm_walk_t w;
        if (vmm_walk(page, &w))
            continue;                               // already there: reuse it
        void *frame = pmm_alloc_block();
        if (!frame || !vmm_map_page(page, (uint32_t)frame, PAGE_WRITABLE | PAGE_USER))
            return 0;
    }
    memset((void *)ph->p_vaddr, 0, ph->p_memsz);
    return !ph->p_filesz ||
           vfs_read(file, ph->p_offset, ph->p_filesz, (uint8_t *)ph->p_vaddr) == ph->p_filesz;
}

static uint32_t stack_area_top(process_t *p) {
    return EXEC_STACKS_START + (uint32_t)(p - process_get_table() + 1) * AREA_SIZE;
}

// The top of 'p's own stack area, its pages mapped. 0 = out of memory.
static uint32_t stack_top(process_t *p) {
    uint32_t top = stack_area_top(p);
    for (uint32_t page = top - EXEC_STACK_PAGES * PAGE_SIZE; page < top; page += PAGE_SIZE) {
        vmm_walk_t w;
        if (vmm_walk(page, &w))
            continue;
        void *frame = pmm_alloc_block();
        if (!frame || !vmm_map_page(page, (uint32_t)frame, PAGE_WRITABLE | PAGE_USER))
            return 0;
    }
    return top;
}

// System V stack: argc, argv[], NULL, envp[], NULL, then the strings.
static uint32_t build_stack(uint32_t top, const args_t *a) {
    uint32_t strings = (top - a->used) & ~3u;
    memcpy((void *)strings, a->text, a->used);
    uint32_t *sp = (uint32_t *)(strings - (1 + a->argc + 1 + a->envc + 1) * 4);
    sp = (uint32_t *)((uint32_t)sp & ~15u);         // 16-byte aligned
    uint32_t *w = sp;
    *w++ = a->argc;
    for (uint32_t i = 0; i < a->argc; i++) *w++ = strings + a->start[i];
    *w++ = 0;
    for (uint32_t i = 0; i < a->envc; i++) *w++ = strings + a->start[a->argc + i];
    *w = 0;
    return (uint32_t)sp;
}

static const char *base_name(const char *path) {
    const char *b = path;
    for (const char *c = path; *c; c++)
        if (*c == '/' && c[1])
            b = c + 1;
    return b;
}

int sys_execve(registers_t *regs, const char *path, char *const argv[], char *const envp[]) {
    process_t *self = scheduler_get_current_process();
    int from_user = (regs->cs & 3) == 3;
    if (!self || !path || !integrity_verify())
        return -1;

    // 1. DEFENCE arg_limits: sizes and pointers, before anything is copied.
    uint32_t count = 0, bytes = 0;
    if (!def_args_ok(argv, from_user, &count, &bytes) || !def_args_ok(envp, from_user, &count, &bytes))
        return -1;                                  // E2BIG / EFAULT
    args_t args = { kmalloc(bytes ? bytes : 1), 0, 0, 0, {0} };
    if (!args.text)
        return -1;
    copy_list(&args, argv, 0);
    copy_list(&args, envp, 1);

    // 2. The file, checked (DEFENCE elf_validate) - still able to fail.
    Elf32_Ehdr eh;
    Elf32_Phdr ph[ELF_MAX_PHDRS];
    vfs_node_t *file = vfs_lookup(vfs_cwd(), path);
    if (!file || !(file->flags & FS_FILE) || !perm_check(file, PERM_READ) ||
        vfs_read(file, 0, sizeof(eh), (uint8_t *)&eh) != sizeof(eh) ||
        !def_elf_check_header(&eh) ||
        vfs_read(file, eh.e_phoff, eh.e_phnum * sizeof(Elf32_Phdr), (uint8_t *)ph) !=
            eh.e_phnum * sizeof(Elf32_Phdr) ||
        !def_elf_check_segments(ph, eh.e_phnum) ||
        // DEFENCE guard_gap: nothing in the stack area, nothing right below our stack.
        !def_segments_clear_of_stacks(ph, eh.e_phnum) ||
        !def_guard_gap_ok(stack_area_top(self) - EXEC_STACK_PAGES * PAGE_SIZE)) {
        kfree(args.text);
        return -1;                                  // ENOENT / EACCES / ENOEXEC
    }

    // 3. The point of no return: the new program goes into memory.
    for (int i = 0; i < eh.e_phnum; i++)
        if (ph[i].p_type == PT_LOAD && !load_segment(file, &ph[i])) {
            kfree(args.text);
            process_exit_with(SIGSEGV);             // half loaded: it cannot go on
        }

    // 4. Its stack, with the arguments.
    uint32_t top = stack_top(self);
    uint32_t sp = top ? build_stack(top, &args) : 0;
    kfree(args.text);
    // DEFENCE user_stack: ring 3 only ever gets a user-space stack.
    if (!sp || !def_user_stack_ok(sp))
        process_exit_with(SIGSEGV);

    // 5. What does not survive an exec.
    def_close_on_exec(self);                        // DEFENCE close_on_exec
    for (int sig = 1; sig < SIGNAL_COUNT; sig++) {
        if (self->sig_handler[sig] != SIG_IGN)
            self->sig_handler[sig] = SIG_DFL;       // its code is gone
        self->sig_flags[sig] = self->sig_restorer[sig] = self->sig_mask[sig] = 0;
    }
    self->sig_frames = 0;
    self->altstack_sp = self->altstack_size = 0;
    strncpy(self->name, base_name(path), PROCESS_NAME_LEN - 1);
    self->name[PROCESS_NAME_LEN - 1] = '\0';
    self->vfork_parent = 0;                         // a vfork() parent may go on

    // 6. Into the new program, in ring 3.
    if (!from_user)
        switch_to_user_mode(eh.e_entry, sp);        // never returns
    regs->eip = eh.e_entry;
    regs->useresp = sp;
    regs->eflags = FLAGS_USER;
    regs->ebx = regs->ecx = regs->edx = regs->esi = regs->edi = regs->ebp = 0;
    return 0;                                       // its EAX
}
