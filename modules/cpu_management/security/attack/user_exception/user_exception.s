; =============================================================================
; user_exception.s - Five ring 3 programs, each runs ONE forbidden instruction.
;
; Used by atk_user_exception.c through usertask (process_management).
; Each one should make the CPU raise an exception. If the CPU ever let
; the instruction through, the program just exits normally, so a
; surprise can never hang the OS.
; =============================================================================

SYS_EXIT equ 1

global uexc_div0, uexc_div0_end
global uexc_ud2,  uexc_ud2_end
global uexc_cli,  uexc_cli_end
global uexc_in,   uexc_in_end
global uexc_hlt,  uexc_hlt_end

section .text

; Divide by zero: exception 0 (#DE).
uexc_div0:
    mov eax, 1
    xor edx, edx
    xor ecx, ecx
    div ecx                     ; edx:eax / 0
    mov eax, SYS_EXIT
    int 0x80
uexc_div0_end:

; An instruction that is "undefined" on purpose: exception 6 (#UD).
uexc_ud2:
    ud2
    mov eax, SYS_EXIT
    int 0x80
uexc_ud2_end:

; Turn interrupts off. With IOPL 0 that is ring 0 only:
; exception 13 (#GP). If it worked, the scheduler would never run again.
uexc_cli:
    cli
    mov eax, SYS_EXIT
    int 0x80
uexc_cli_end:

; Read the keyboard's port directly. Ports are ring 0 only (IOPL 0, and
; the TSS has no I/O bitmap): exception 13 (#GP).
uexc_in:
    in al, 0x60
    mov eax, SYS_EXIT
    int 0x80
uexc_in_end:

; Stop the CPU. Ring 0 only: exception 13 (#GP).
uexc_hlt:
    hlt
    mov eax, SYS_EXIT
    int 0x80
uexc_hlt_end:
