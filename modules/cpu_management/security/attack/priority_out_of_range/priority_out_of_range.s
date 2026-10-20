; =============================================================================
; priority_out_of_range.s - A ring 3 program (user 1) asks for bad priorities.
;
; Used by atk_priority_out_of_range.c through usertask (process_management).
; Each setpriority() result (0 = done, -1 = refused) goes into the
; mailbox, so the kernel can show them after the program exits.
; =============================================================================

SYS_EXIT        equ 1
SYS_SETPRIORITY equ 21
MAILBOX         equ 0x40000F00  ; = USERTASK_MAILBOX in usertask.h
DONE            equ 0x600D      ; mailbox[0]: "all tries finished"

global prio_program, prio_program_end

section .text

; try pid, level, slot:  mailbox[slot] = setpriority(pid, level)
%macro TRY 3
    mov eax, SYS_SETPRIORITY
    mov ebx, %1
    mov ecx, %2
    int 0x80
    mov [MAILBOX + 4 * %3], eax
%endmacro

prio_program:
    TRY 0, 1000, 1              ; me, far above the top level (7)
    TRY 0, -5,   2              ; me, below the idle task
    TRY 0, 7,    3              ; me, the top level: only root may raise
    TRY 2, 1,    4              ; kernel_main (the shell), down to the bottom
    TRY 1, 7,    5              ; the idle task, to the top

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    int 0x80
prio_program_end:
