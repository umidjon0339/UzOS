; =============================================================================
; debug.s - A ring 3 debugger: ptrace a child, read and write memory of
; processes, compare kernel objects (kcmp).
;
; Run by cmd_proccalls.c (group "debug") through usertask, as user 1.
; Child 1 waits in pause(); we attach (it stops), peek and poke its
; memory, read its registers (it is inside pause: orig_eax = 78), let go,
; attach again and kill it. Child 2 asks its parent to trace it (TRACEME).
; =============================================================================

SYS_EXIT              equ 1
SYS_SLEEP             equ 3
SYS_CLONE             equ 43
SYS_PAUSE             equ 78
SYS_WAIT4             equ 81
SYS_GETPID            equ 2
SYS_PTRACE            equ 93
SYS_PROCESS_VM_READV  equ 94
SYS_PROCESS_VM_WRITEV equ 95
SYS_KCMP              equ 96

PTRACE_TRACEME  equ 0
PTRACE_PEEKDATA equ 2
PTRACE_POKEDATA equ 5
PTRACE_KILL     equ 8
PTRACE_GETREGS  equ 12
PTRACE_ATTACH   equ 16
PTRACE_DETACH   equ 17
KCMP_VM         equ 1
KCMP_FILES      equ 2
CHILD_FLAGS     equ 0x100 | 17

MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516
VAR         equ SCRATCH + 0         ; the word we peek / poke
PEEKED         equ SCRATCH + 4
KID         equ SCRATCH + 8
MYPID       equ SCRATCH + 12
STATUS      equ SCRATCH + 16
LOCAL       equ SCRATCH + 20        ; iovec
REMOTE      equ SCRATCH + 28        ; iovec
SRC         equ SCRATCH + 36
DST         equ SCRATCH + 40
BUF         equ SCRATCH + 44
REGS        equ SCRATCH + 64        ; ptrace_regs_t (68)

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; ptrace(request, pid, addr, data) -> slot
%macro PTRACE 5
    mov eax, SYS_PTRACE
    mov ebx, %1
    mov ecx, %2
    mov edx, %3
    mov esi, %4
    int 0x80
    STORE %5
%endmacro

; process_vm_<kind>(pid, LOCAL, 1, REMOTE, 1, 0) -> slot
%macro VMCOPY 3
    mov eax, %1
    mov ebx, %2
    mov ecx, LOCAL
    mov edx, 1
    mov esi, REMOTE
    mov edi, 1
    xor ebp, ebp
    int 0x80
    STORE %3
%endmacro

%macro KCMP 4
    mov eax, SYS_KCMP
    mov ebx, %1
    mov ecx, %2
    mov edx, %3
    xor esi, esi
    xor edi, edi
    int 0x80
    STORE %4
%endmacro

%macro CLONE 1
    mov eax, SYS_CLONE
    mov ebx, CHILD_FLAGS
    mov ecx, %1
    xor edx, edx
    xor edi, edi
    int 0x80
%endmacro

%macro WAIT4 1
    mov eax, SYS_WAIT4
    mov ebx, [KID]
    mov ecx, STATUS
    xor edx, edx
    xor esi, esi
    int 0x80
    STORE %1
%endmacro

global debug_program, debug_program_end

section .text

debug_program:
    mov edi, SCRATCH
    mov ecx, 48
    xor eax, eax
    rep stosd
    mov dword [VAR], 0x1234
    mov eax, SYS_GETPID
    int 0x80
    mov [MYPID], eax

    ; ---- ptrace a child ----
    CLONE 0x40001800
    test eax, eax
    jz child_pause
    mov [KID], eax
    STORE 1
    mov eax, SYS_SLEEP                                  ; let it reach pause()
    mov ebx, 50
    int 0x80
    PTRACE PTRACE_ATTACH, [KID], 0, 0, 2
    PTRACE PTRACE_ATTACH, 1, 0, 0, 3                    ; a system process
    PTRACE PTRACE_PEEKDATA, [KID], VAR, PEEKED, 4
    mov eax, [PEEKED]
    STORE 5
    PTRACE PTRACE_POKEDATA, [KID], VAR, 0x5678, 6
    mov eax, [VAR]
    STORE 7
    PTRACE PTRACE_GETREGS, [KID], 0, REGS, 8
    mov eax, [REGS + 44]                                ; orig_eax
    STORE 9
    mov eax, [REGS + 48]                                ; eip
    STORE 10
    PTRACE PTRACE_PEEKDATA, [KID], 0x00100000, PEEKED, 11  ; kernel memory
    PTRACE PTRACE_DETACH, [KID], 0, 0, 12
    PTRACE PTRACE_PEEKDATA, [KID], VAR, PEEKED, 13         ; not ours any more
    PTRACE PTRACE_ATTACH, [KID], 0, 0, 14
    PTRACE PTRACE_KILL, [KID], 0, 0, 15
    WAIT4 16
    mov eax, [STATUS]
    STORE 17

    ; ---- process_vm_writev / readv ----
    mov dword [SRC], 0x64636261                         ; "abcd"
    mov dword [LOCAL + 0], SRC
    mov dword [LOCAL + 4], 4
    mov dword [REMOTE + 0], DST
    mov dword [REMOTE + 4], 4
    VMCOPY SYS_PROCESS_VM_WRITEV, [MYPID], 18
    mov dword [LOCAL + 0], BUF
    VMCOPY SYS_PROCESS_VM_READV, [MYPID], 19
    mov eax, [BUF]
    STORE 20
    VMCOPY SYS_PROCESS_VM_READV, 1, 21                  ; idle's memory

    ; ---- kcmp ----
    KCMP [MYPID], [MYPID], KCMP_VM, 22
    KCMP [MYPID], [MYPID], KCMP_FILES, 23
    KCMP [MYPID], 1, KCMP_VM, 24                        ; idle: hidden from us
    KCMP [MYPID], [MYPID], 99, 25

    ; ---- TRACEME ----
    CLONE 0x40001A00
    test eax, eax
    jz child_traceme
    mov [KID], eax
    WAIT4 27

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

child_pause:
    mov eax, SYS_PAUSE
    int 0x80
    jmp child_pause

child_traceme:
    PTRACE PTRACE_TRACEME, 0, 0, 0, 26
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
debug_program_end:
