; =============================================================================
; syscall_abuse.s - A "taken over" ring 3 program inside a seccomp sandbox.
;
; Used by atk_syscall_abuse.c through usertask (process_management).
; The honest part of the program first gives up what it does not need:
;   prctl(PR_SET_NO_NEW_PRIVS, 1)
;   seccomp(FILTER, allow exit, getpid, write, rt_sigreturn, seccomp;
;           anything else fails with -1)
; Then the "attacker's code" tries the dangerous calls:
;   open("/home/guest/stolen")  kill(1, SIGKILL)  setuid(0)
; and last, after seccomp(STRICT), getpid() - which must end the program.
; Mailbox: [1..3] = the three answers, [4] = 1 if getpid survived
; strict mode, [0] = 1 when the sandbox was set up.
; =============================================================================

SYS_EXIT     equ 1
SYS_GETPID   equ 2
SYS_OPEN     equ 14
SYS_SETUID   equ 24
SYS_KILL     equ 58
SYS_PRCTL    equ 80
SYS_SECCOMP  equ 90
PR_SET_NNP   equ 38
SET_STRICT   equ 0
SET_FILTER   equ 1
RET_ERRNO_1  equ 0x00050001

USER_CODE    equ 0x40000000
MAILBOX      equ 0x40000F00
SCRATCH      equ 0x40001000

%define ADDR(label) (USER_CODE + (label - abuse_program))

global abuse_program, abuse_program_end

section .text

abuse_program:
    mov eax, SYS_PRCTL
    mov ebx, PR_SET_NNP
    mov ecx, 1
    int 0x80
    ; the list: action, then 128 bits (syscalls 1, 2, 16, 76, 90)
    mov dword [SCRATCH + 0], RET_ERRNO_1
    mov dword [SCRATCH + 4], (1 << 1) | (1 << 2) | (1 << 16)
    mov dword [SCRATCH + 8], 0
    mov dword [SCRATCH + 12], (1 << (76 - 64)) | (1 << (90 - 64))
    mov dword [SCRATCH + 16], 0
    mov eax, SYS_SECCOMP
    mov ebx, SET_FILTER
    xor ecx, ecx
    mov edx, SCRATCH
    int 0x80
    test eax, eax
    jnz .exit                           ; no sandbox: nothing to test
    mov dword [MAILBOX], 1

    ; ---- the attacker's code ----
    mov eax, SYS_OPEN
    mov ebx, ADDR(path)
    mov ecx, 0x06                       ; O_WRITE | O_CREATE
    int 0x80
    mov [MAILBOX + 4], eax
    mov eax, SYS_KILL
    mov ebx, 1
    mov ecx, 9
    int 0x80
    mov [MAILBOX + 8], eax
    mov eax, SYS_SETUID
    xor ebx, ebx
    int 0x80
    mov [MAILBOX + 12], eax

    ; ---- strict mode: getpid is no longer allowed ----
    mov eax, SYS_SECCOMP
    mov ebx, SET_STRICT
    xor ecx, ecx
    xor edx, edx
    int 0x80
    mov eax, SYS_GETPID
    int 0x80
    mov dword [MAILBOX + 16], 1         ; never: SIGSYS ended us
.exit:
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

path: db "/home/guest/stolen", 0
abuse_program_end:
