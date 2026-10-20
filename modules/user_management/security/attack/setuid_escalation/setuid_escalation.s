; =============================================================================
; setuid_escalation.s - A ring 3 program (guest, uid 1) asks to become root.
;
; Used by atk_setuid_escalation.c through usertask (process_management).
; It calls setuid(0), then getuid() to see who it is now, and leaves
; both answers in the mailbox for the kernel to show.
; =============================================================================

SYS_EXIT   equ 1
SYS_GETUID equ 23
SYS_SETUID equ 24
MAILBOX    equ 0x40000F00       ; = USERTASK_MAILBOX in usertask.h
DONE       equ 0x600D           ; mailbox[0]: "finished"

global setuid_program, setuid_program_end

section .text

setuid_program:
    mov eax, SYS_SETUID
    mov ebx, 0                  ; "make me root"
    int 0x80
    mov [MAILBOX + 4], eax      ; mailbox[1] = 0 (done) or -1 (refused)

    mov eax, SYS_GETUID
    int 0x80
    mov [MAILBOX + 8], eax      ; mailbox[2] = who I am now

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    int 0x80
setuid_program_end:
