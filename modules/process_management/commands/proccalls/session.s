; =============================================================================
; session.s - A ring 3 program that moves between process groups and
; signals a whole group: getppid, getpgrp, getsid, setpgid, getpgid,
; setsid, kill(0 / -pgid), wait4.
;
; Run by cmd_proccalls.c (group "session") through usertask, as user 1.
; Its child (clone CLONE_VM | SIGCHLD) waits in pause() until the group
; kill ends it.
; =============================================================================

SYS_EXIT    equ 1
SYS_CLONE   equ 43
SYS_GETPPID equ 52
SYS_SETPGID equ 53
SYS_GETPGID equ 54
SYS_GETPGRP equ 55
SYS_SETSID  equ 56
SYS_GETSID  equ 57
SYS_KILL    equ 58
SYS_PAUSE   equ 78
SYS_WAIT4   equ 81

CHILD_FLAGS equ 0x100 | 17          ; CLONE_VM, exit signal SIGCHLD
SIGKILL     equ 9

MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516
STATUS      equ SCRATCH + 0
KID         equ SCRATCH + 4

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; syscall with up to two arguments -> slot
%macro CALL2 4
    mov eax, %1
    mov ebx, %2
    mov ecx, %3
    int 0x80
    STORE %4
%endmacro

global session_program, session_program_end

section .text

session_program:
    CALL2 SYS_GETPPID, 0, 0, 1
    CALL2 SYS_GETPGRP, 0, 0, 2
    CALL2 SYS_GETSID, 0, 0, 3
    CALL2 SYS_SETPGID, 0, 1, 4          ; idle's group: another session
    CALL2 SYS_SETPGID, 0, 0, 5          ; a new group of our own
    CALL2 SYS_GETPGRP, 0, 0, 6
    CALL2 SYS_SETSID, 0, 0, 7           ; a group leader may not

    mov eax, SYS_CLONE
    mov ebx, CHILD_FLAGS
    mov ecx, 0x40001800                 ; the child's stack top
    xor edx, edx
    xor edi, edi
    int 0x80
    test eax, eax
    jz child
    mov [KID], eax
    STORE 8

    CALL2 SYS_SETPGID, [KID], 0, 9      ; the child: a group of its own
    CALL2 SYS_GETPGID, [KID], 0, 10
    mov ecx, [KID]
    neg ecx
    CALL2 SYS_KILL, ecx, 0, 11          ; kill(-group, 0): it exists
    mov ecx, [KID]
    neg ecx
    CALL2 SYS_KILL, ecx, SIGKILL, 12    ; kill(-group, SIGKILL)
    mov eax, SYS_WAIT4
    mov ebx, [KID]
    mov ecx, STATUS
    xor edx, edx
    xor esi, esi
    int 0x80
    STORE 13
    mov eax, [STATUS]
    STORE 14
    CALL2 SYS_KILL, 0, 0, 15            ; kill(0, 0): my own group
    CALL2 SYS_KILL, -99999, 0, 16       ; no such group

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

child:
    mov eax, SYS_PAUSE
    int 0x80
    jmp child
session_program_end:
