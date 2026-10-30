; =============================================================================
; sandbox.s - A ring 3 program that tries unshare / setns and then locks
; itself in a seccomp sandbox, step by step.
;
; Run by cmd_proccalls.c (group "sandbox") through usertask, as user 1.
; The last syscall (getpid after seccomp STRICT) must END the program:
; its slot stays 0. DONE is written to the mailbox just before it.
; =============================================================================

SYS_EXIT     equ 1
SYS_GETPID   equ 2
SYS_OPEN     equ 14
SYS_CLONE    equ 43
SYS_KILL     equ 58
SYS_PRCTL    equ 80
SYS_WAIT4    equ 81
SYS_SECCOMP  equ 90
SYS_UNSHARE  equ 91
SYS_SETNS    equ 92
PR_SET_NNP   equ 38
PR_GET_NNP   equ 39
SET_STRICT   equ 0
SET_FILTER   equ 1
GET_ACTION   equ 2
RET_ERRNO_1  equ 0x00050001
CHILD_FLAGS  equ 0x100 | 17

USER_CODE    equ 0x40000000
MAILBOX      equ 0x40000F00
SCRATCH      equ 0x40001000
DONE         equ 0x516
LIST         equ SCRATCH + 0        ; seccomp_list_t (20)
ACTION       equ SCRATCH + 32
KID          equ SCRATCH + 36

%define ADDR(label) (USER_CODE + (label - sandbox_program))
%define BIT(n) (1 << ((n) % 32))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

%macro CALL3 5
    mov eax, %1
    mov ebx, %2
    mov ecx, %3
    mov edx, %4
    int 0x80
    STORE %5
%endmacro

; the filter list: action, then the allowed syscalls 0..127
%macro LISTSET 5
    mov dword [LIST + 0], %1
    mov dword [LIST + 4], %2
    mov dword [LIST + 8], %3
    mov dword [LIST + 12], %4
    mov dword [LIST + 16], %5
%endmacro

global sandbox_program, sandbox_program_end

section .text

sandbox_program:
    CALL3 SYS_UNSHARE, 0, 0, 0, 1
    CALL3 SYS_UNSHARE, 0x400, 0, 0, 2           ; CLONE_FILES: already private
    CALL3 SYS_UNSHARE, 0x20000000, 0, 0, 3      ; CLONE_NEWPID: no namespaces
    CALL3 SYS_SETNS, 3, 0, 0, 4
    mov dword [ACTION], RET_ERRNO_1
    CALL3 SYS_SECCOMP, GET_ACTION, 0, ACTION, 5

    ; allowed: exit 1, getpid 2, write 16, clone 43, rt_sigreturn 76,
    ;          prctl 80, wait4 81, seccomp 90
    LISTSET RET_ERRNO_1, BIT(1) | BIT(2) | BIT(16), BIT(43), BIT(76) | BIT(80) | BIT(81) | BIT(90), 0
    CALL3 SYS_SECCOMP, SET_FILTER, 0, LIST, 6   ; no no_new_privs yet
    CALL3 SYS_PRCTL, PR_SET_NNP, 1, 0, 7
    CALL3 SYS_SECCOMP, SET_FILTER, 0, LIST, 8
    CALL3 SYS_GETPID, 0, 0, 0, 9                ; allowed
    CALL3 SYS_OPEN, ADDR(path), 0x01, 0, 10     ; not allowed: -1
    CALL3 SYS_KILL, 1, 0, 0, 11                 ; not allowed: -1

    ; a child inherits the sandbox and the promise
    mov eax, SYS_CLONE
    mov ebx, CHILD_FLAGS
    mov ecx, 0x40001800
    xor edx, edx
    xor edi, edi
    int 0x80
    test eax, eax
    jz child
    mov [KID], eax
    STORE 12
    mov eax, SYS_WAIT4
    mov ebx, [KID]
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    int 0x80
    STORE 15

    ; a looser filter cannot loosen anything
    LISTSET RET_ERRNO_1, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
    CALL3 SYS_SECCOMP, SET_FILTER, 0, LIST, 16
    CALL3 SYS_OPEN, ADDR(path), 0x01, 0, 17     ; still -1

    CALL3 SYS_SECCOMP, SET_STRICT, 0, 0, 18
    mov dword [MAILBOX], DONE
    CALL3 SYS_GETPID, 0, 0, 0, 19               ; SIGSYS: never stored
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

child:
    CALL3 SYS_PRCTL, PR_GET_NNP, 0, 0, 13
    CALL3 SYS_OPEN, ADDR(path), 0x01, 0, 14     ; the child is sandboxed too
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

path: db "/home/guest", 0
sandbox_program_end:
