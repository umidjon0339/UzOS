; =============================================================================
; exec.s - A ring 3 program that starts another program: vfork + execve.
;
; Run by cmd_proccalls.c (group "exec") through usertask, as user 1.
; cmd_proccalls.c wrote /home/guest/showargs (showargs.s, a hand-built
; ELF) before starting it. First the refusals, then the real thing:
;   vfork -> child: execve("/home/guest/showargs",
;                          ["showargs", "one", "two"],
;                          ["HOME=/home/guest", "LD_PRELOAD=/evil.so", "NOEQ"])
; The child becomes showargs (same pid), which reports what it got in
; mailbox [40..45] and exits with 42; we copy that to slots 10..15.
; =============================================================================

SYS_EXIT     equ 1
SYS_OPEN     equ 14
SYS_WRITE    equ 16
SYS_UNLINK   equ 10
SYS_EXECVE   equ 67
SYS_EXECVEAT equ 68
SYS_FORK     equ 65
SYS_VFORK    equ 66
SYS_WAIT4    equ 81
O_SECRET     equ 0x01 | 0x02 | 0x04 | 0x80000   ; read, write, create, O_CLOEXEC

USER_CODE    equ 0x40000000
MAILBOX      equ 0x40000F00
SCRATCH      equ 0x40001000
DONE         equ 0x516
STATUS       equ SCRATCH + 0
KID          equ SCRATCH + 4
MANY         equ SCRATCH + 16       ; 41 pointers + NULL

%define ADDR(label) (USER_CODE + (label - exec_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; execve(path, argv, envp) -> slot
%macro EXECVE 4
    mov eax, SYS_EXECVE
    mov ebx, %1
    mov ecx, %2
    mov edx, %3
    int 0x80
    STORE %4
%endmacro

global exec_program, exec_program_end

section .text

exec_program:
    ; a secret file, opened close-on-exec
    mov eax, SYS_OPEN
    mov ebx, ADDR(secret)
    mov ecx, O_SECRET
    int 0x80
    STORE 1
    mov ebx, eax
    mov eax, SYS_WRITE
    mov ecx, ADDR(secret)
    mov edx, 1
    int 0x80
    STORE 2

    ; ---- refusals: the old program carries on ----
    EXECVE ADDR(nowhere), ADDR(argv_ok), 0, 3       ; no such file
    mov edi, MANY                                   ; 41 x "x": too many
    mov ecx, 41
.fill:
    mov dword [edi], ADDR(x)
    add edi, 4
    loop .fill
    mov dword [edi], 0
    EXECVE ADDR(prog), MANY, 0, 4                   ; arg_limits
    EXECVE ADDR(prog), ADDR(argv_kernel), 0, 5      ; a pointer into the kernel
    EXECVE ADDR(secret), ADDR(argv_ok), 0, 6        ; not an ELF: elf_validate

    ; ---- vfork + execve ----
    mov eax, SYS_VFORK
    int 0x80
    test eax, eax
    jz child
    mov [KID], eax
    STORE 7
    mov eax, SYS_WAIT4
    mov ebx, [KID]
    mov ecx, STATUS
    xor edx, edx
    xor esi, esi
    int 0x80
    STORE 8
    mov eax, [STATUS]
    STORE 9
    mov esi, MAILBOX + 40 * 4                       ; what showargs reported
    mov edi, MAILBOX + 10 * 4
    mov ecx, 6
    rep movsd

    mov eax, SYS_FORK
    int 0x80
    STORE 16                                        ; -1: one address space
    mov eax, SYS_EXECVEAT
    mov ebx, 5                                      ; a descriptor, not AT_FDCWD
    mov ecx, ADDR(relpath)
    mov edx, ADDR(argv_ok)
    xor esi, esi
    xor edi, edi
    int 0x80
    STORE 17
    mov eax, SYS_UNLINK
    mov ebx, ADDR(secret)
    int 0x80

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

child:
    EXECVE ADDR(prog), ADDR(argv_ok), ADDR(envp_mixed), 39
    mov eax, SYS_EXIT                               ; only if execve failed
    mov ebx, 127
    int 0x80

prog:     db "/home/guest/showargs", 0
nowhere:  db "/no/such/program", 0
secret:   db "/home/guest/secret", 0
relpath:  db "showargs", 0
x:        db "x", 0
a0:       db "showargs", 0
a1:       db "one", 0
a2:       db "two", 0
e0:       db "HOME=/home/guest", 0
e1:       db "LD_PRELOAD=/evil.so", 0
e2:       db "NOEQ", 0
align 4
argv_ok:     dd ADDR(a0), ADDR(a1), ADDR(a2), 0
argv_kernel: dd ADDR(a0), 0x00100000, 0
envp_mixed:  dd ADDR(e0), ADDR(e1), ADDR(e2), 0
exec_program_end:
