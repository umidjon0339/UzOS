; =============================================================================
; process_masquerade.s - A ring 3 program that tries to rename itself.
;
; Used by atk_process_masquerade.c through usertask (process_management).
;   1. prctl(PR_SET_NAME, "kernel_main")     pose as the kernel
;   2. prctl(PR_SET_NAME, ESC "[2J" "x")     a name that clears the screen
;   3. prctl(PR_SET_NAME, "masq_ok")         a normal rename (must work)
; Mailbox: [1..3] = the three answers, [4] = the name it ended with
; (PR_GET_NAME, first 4 bytes), [0] = 1 when done.
; =============================================================================

SYS_EXIT    equ 1
SYS_PRCTL   equ 80
PR_SET_NAME equ 15
PR_GET_NAME equ 16

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000

%define ADDR(label) (USER_CODE + (label - masq_program))

%macro RENAME 2
    mov eax, SYS_PRCTL
    mov ebx, PR_SET_NAME
    mov ecx, ADDR(%1)
    int 0x80
    mov [MAILBOX + %2 * 4], eax
%endmacro

global masq_program, masq_program_end

section .text

masq_program:
    RENAME kernel_name, 1
    RENAME escape_name, 2
    RENAME normal_name, 3
    mov eax, SYS_PRCTL
    mov ebx, PR_GET_NAME
    mov ecx, SCRATCH
    int 0x80
    mov eax, [SCRATCH]
    mov [MAILBOX + 16], eax
    mov dword [MAILBOX], 1
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

kernel_name: db "kernel_main", 0
escape_name: db 27, "[2Jx", 0
normal_name: db "masq_ok", 0
masq_program_end:
