; =============================================================================
; no_new_privs_bypass.s - A ring 3 program that tries to get around the
; no_new_privs promise.
;
; Used by atk_no_new_privs_bypass.c through usertask, as user 1.
;   1. seccomp(FILTER) WITHOUT no_new_privs        -> must fail
;   2. prctl(PR_SET_NO_NEW_PRIVS, 1)               -> 0
;   3. prctl(PR_SET_NO_NEW_PRIVS, 0): take it back -> must fail
;   4. a child (clone): does it still have it?     -> must be 1
; Mailbox: [1] [2] [3] = answers 1-3, [4] = the child's
; PR_GET_NO_NEW_PRIVS, [0] = 1 when done.
; =============================================================================

SYS_EXIT    equ 1
SYS_CLONE   equ 43
SYS_PRCTL   equ 80
SYS_WAIT4   equ 81
SYS_SECCOMP equ 90
PR_SET_NNP  equ 38
PR_GET_NNP  equ 39
SET_FILTER  equ 1
CHILD_FLAGS equ 0x100 | 17          ; CLONE_VM, SIGCHLD

MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000

global nnp_program, nnp_program_end

section .text

nnp_program:
    mov dword [SCRATCH + 0], 0x00050001     ; a filter: refuse with -1 ...
    mov dword [SCRATCH + 4], 0xFFFFFFFF     ; ... nothing (all allowed)
    mov dword [SCRATCH + 8], 0xFFFFFFFF
    mov dword [SCRATCH + 12], 0xFFFFFFFF
    mov dword [SCRATCH + 16], 0xFFFFFFFF
    mov eax, SYS_SECCOMP
    mov ebx, SET_FILTER
    xor ecx, ecx
    mov edx, SCRATCH
    int 0x80
    mov [MAILBOX + 4], eax
    mov eax, SYS_PRCTL
    mov ebx, PR_SET_NNP
    mov ecx, 1
    int 0x80
    mov [MAILBOX + 8], eax
    mov eax, SYS_PRCTL
    mov ebx, PR_SET_NNP
    xor ecx, ecx
    int 0x80
    mov [MAILBOX + 12], eax

    mov eax, SYS_CLONE
    mov ebx, CHILD_FLAGS
    mov ecx, 0x40001800
    xor edx, edx
    xor edi, edi
    int 0x80
    test eax, eax
    jz .child
    mov ebx, eax                        ; wait4(child)
    mov eax, SYS_WAIT4
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    int 0x80
    mov dword [MAILBOX], 1
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

.child:
    mov eax, SYS_PRCTL
    mov ebx, PR_GET_NNP
    int 0x80
    mov [MAILBOX + 16], eax
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
nnp_program_end:
