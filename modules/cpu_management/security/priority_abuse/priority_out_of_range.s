; =============================================================================
; priority_out_of_range.s - A ring 3 program (user 1) asks for bad priorities.
;
; Used by atk_priority_out_of_range.c through usertask (process_management).
; Each answer (0 = done, -1 = refused) goes into the mailbox, so the
; kernel can show them after the program exits.
;
; Numbers match modules/cpu_management/syscalls/<name>/sys_<name>.h.
; =============================================================================

SYS_EXIT               equ 1
SYS_SETPRIORITY        equ 21   ; setpriority(which, who, nice)
SYS_SCHED_SETSCHEDULER equ 26   ; sched_setscheduler(pid, policy, &param)
PRIO_PROCESS           equ 0
SCHED_FIFO             equ 1
MAILBOX                equ 0x40000F00  ; = USERTASK_MAILBOX in usertask.h
DONE                   equ 0x600D      ; mailbox[0]: "all tries finished"

global prio_program, prio_program_end

section .text

; try who, nice, slot:  mailbox[slot] = setpriority(PRIO_PROCESS, who, nice)
%macro TRY 3
    mov eax, SYS_SETPRIORITY
    mov ebx, PRIO_PROCESS
    mov ecx, %1
    mov edx, %2
    int 0x80
    mov [MAILBOX + 4 * %3], eax
%endmacro

prio_program:
    TRY 0, 1000, 1              ; me, far above the top (19)
    TRY 0, -500, 2              ; me, far below the bottom (-20)
    TRY 0, -20,  3              ; me, the greediest nice: only root may lower
    TRY 2, 19,   4              ; kernel_main (the shell), to the very bottom
    TRY 1, -20,  5              ; the idle task, to the top

    ; mailbox[6] = sched_setscheduler(0, SCHED_FIFO, &{99})
    push 99                     ; struct sched_param { sched_priority = 99 }
    mov eax, SYS_SCHED_SETSCHEDULER
    mov ebx, 0                  ; me
    mov ecx, SCHED_FIFO         ; real-time: runs before every normal process
    mov edx, esp                ; &param
    int 0x80
    add esp, 4
    mov [MAILBOX + 4 * 6], eax

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    int 0x80
prio_program_end:
