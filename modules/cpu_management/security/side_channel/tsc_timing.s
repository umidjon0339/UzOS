; =============================================================================
; tsc_timing.s - A ring 3 program that times a system call with rdtsc.
;
; Used by atk_tsc_timing.c through usertask (process_management).
; It reads the cycle counter, asks the kernel for its PID, reads the
; counter again, and leaves the difference in the mailbox for the
; kernel to show. With tsc_restrict on it never gets that far: the very
; first rdtsc raises #13 and the program is killed.
; =============================================================================

SYS_EXIT   equ 1
SYS_GETPID equ 2
MAILBOX    equ 0x40000F00       ; = USERTASK_MAILBOX in usertask.h

global tsc_timing_program, tsc_timing_program_end

section .text

tsc_timing_program:
    rdtsc                       ; edx:eax = cycles (refused in ring 3?)
    mov esi, eax                ; keep the low 32 bits: enough for a short call

    mov eax, SYS_GETPID
    int 0x80                    ; the kernel work we want to time

    rdtsc
    sub eax, esi                ; cycles the syscall took
    mov [MAILBOX + 4], eax      ; mailbox[1] = cycles
    mov dword [MAILBOX], 1      ; mailbox[0] = 1: "I measured it"

    mov eax, SYS_EXIT
    int 0x80
tsc_timing_program_end:
