; =============================================================================
; prctl.s - A ring 3 program that calls prctl() with every option UzOS has.
;
; Run by cmd_proccalls.c (group "prctl") through usertask, as user 1.
; Mailbox: [1..] = each answer, [0] = DONE at the end.
; Numbers match modules/process_management/syscalls/prctl/sys_prctl.h.
; =============================================================================

SYS_EXIT     equ 1
SYS_PRCTL    equ 80
PR_SET_PDEATHSIG       equ 1
PR_GET_PDEATHSIG       equ 2
PR_GET_DUMPABLE        equ 3
PR_SET_DUMPABLE        equ 4
PR_SET_NAME            equ 15
PR_GET_NAME            equ 16
PR_SET_CHILD_SUBREAPER equ 36
PR_GET_CHILD_SUBREAPER equ 37
PR_SET_NO_NEW_PRIVS    equ 38
PR_GET_NO_NEW_PRIVS    equ 39

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516

%define ADDR(label) (USER_CODE + (label - prctl_program))

; prctl(option, arg2) -> mailbox slot
%macro PRCTL 3
    mov eax, SYS_PRCTL
    mov ebx, %1
    mov ecx, %2
    int 0x80
    mov [MAILBOX + %3 * 4], eax
%endmacro

global prctl_program, prctl_program_end

section .text

prctl_program:
    mov edi, SCRATCH
    mov ecx, 16
    xor eax, eax
    rep stosd

    PRCTL PR_SET_NAME, ADDR(good_name), 1
    PRCTL PR_GET_NAME, SCRATCH, 2
    mov eax, [SCRATCH]
    mov [MAILBOX + 3 * 4], eax                  ; "pc_o" as 4 bytes
    PRCTL PR_SET_NAME, ADDR(kernel_name), 4     ; name_guard: refused
    PRCTL PR_SET_PDEATHSIG, 15, 5
    PRCTL PR_GET_PDEATHSIG, SCRATCH + 16, 6
    mov eax, [SCRATCH + 16]
    mov [MAILBOX + 7 * 4], eax
    PRCTL PR_SET_PDEATHSIG, 99, 8               ; no such signal
    PRCTL PR_GET_DUMPABLE, 0, 9
    PRCTL PR_SET_DUMPABLE, 0, 10
    PRCTL PR_GET_DUMPABLE, 0, 11
    PRCTL PR_SET_DUMPABLE, 1, 12
    PRCTL PR_SET_CHILD_SUBREAPER, 1, 13
    PRCTL PR_GET_CHILD_SUBREAPER, SCRATCH + 20, 14
    mov eax, [SCRATCH + 20]
    mov [MAILBOX + 15 * 4], eax
    PRCTL PR_GET_NO_NEW_PRIVS, 0, 16
    PRCTL PR_SET_NO_NEW_PRIVS, 2, 17            ; only 1 is allowed
    PRCTL PR_SET_NO_NEW_PRIVS, 1, 18
    PRCTL PR_GET_NO_NEW_PRIVS, 0, 19
    PRCTL 12345, 0, 20                          ; no such option

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

good_name:   db "pc_ok", 0
kernel_name: db "kernel_main", 0
prctl_program_end:
