; =============================================================================
; wait.s - A ring 3 program that makes children and waits for them: wait4,
; waitid, waitpid, wait, getrusage, times, pidfd_open / send_signal / getfd.
;
; Run by cmd_proccalls.c (group "wait") through usertask, as user 1.
; Children are made with clone(CLONE_VM | SIGCHLD): a real child process
; (not a thread), sharing our memory, each on its own small stack.
;
; Memory:  MAILBOX 0x40000F00   SCRATCH 0x40001000 (structs)
;          child stacks: tops at 0x40001400, 0x40001600, 0x40001800, 0x40001A00
;          our own stack top 0x40002000
; =============================================================================

SYS_EXIT              equ 1
SYS_GETPID            equ 2
SYS_UNLINK            equ 10
SYS_OPEN              equ 14
SYS_READ              equ 15
SYS_WRITE             equ 16
SYS_CLOSE             equ 17
SYS_SEEK              equ 18
SYS_CLONE             equ 43
SYS_KILL              equ 58
SYS_PIDFD_SEND_SIGNAL equ 61
SYS_WAIT              equ 62
SYS_WAITPID           equ 63
SYS_WAITID            equ 64
SYS_PAUSE             equ 78
SYS_WAIT4             equ 81
SYS_GETRUSAGE         equ 82
SYS_TIMES             equ 83
SYS_PIDFD_OPEN        equ 84
SYS_PIDFD_GETFD       equ 85

CHILD_FLAGS equ 0x100 | 17          ; CLONE_VM, exit signal SIGCHLD
WNOHANG     equ 1
WEXITED     equ 4
P_PIDFD     equ 3
SIGKILL     equ 9
SIGTERM     equ 15
O_RWC       equ 0x01 | 0x02 | 0x04  ; O_READ | O_WRITE | O_CREATE

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516

STATUS  equ SCRATCH + 0         ; int
RUSAGE  equ SCRATCH + 4         ; struct rusage (72)
INFO    equ SCRATCH + 80        ; waitid info (24)
TMS     equ SCRATCH + 104       ; struct tms (16)
BUF     equ SCRATCH + 120       ; 4 bytes
MYPID   equ SCRATCH + 124
PFD     equ SCRATCH + 128       ; a pidfd
FILEFD  equ SCRATCH + 132
COPYFD  equ SCRATCH + 136
KID     equ SCRATCH + 140       ; the newest child's pid

%define ADDR(label) (USER_CODE + (label - wait_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; clone a child onto stack top %1 that runs code %2; parent: eax = pid
%macro CHILD 2
    mov eax, SYS_CLONE
    mov ebx, CHILD_FLAGS
    mov ecx, %1
    xor edx, edx
    xor edi, edi
    int 0x80
    test eax, eax
    jz %2
    mov [KID], eax
%endmacro

global wait_program, wait_program_end

section .text

wait_program:
    mov edi, SCRATCH
    mov ecx, 64
    xor eax, eax
    rep stosd
    mov eax, SYS_GETPID
    int 0x80
    mov [MYPID], eax

    ; ---- 1. a child that exits with 7 ----
    CHILD 0x40001400, child_exit7
    STORE 1
    mov eax, SYS_WAIT4
    mov ebx, [KID]
    mov ecx, STATUS
    xor edx, edx
    mov esi, RUSAGE
    int 0x80
    STORE 2                             ; its pid
    mov eax, [STATUS]
    STORE 3                             ; 7 << 8
    mov eax, [RUSAGE + 16]
    STORE 4                             ; maxrss: 4 KiB
    mov eax, SYS_WAIT4
    mov ebx, -1
    xor ecx, ecx
    mov edx, WNOHANG
    xor esi, esi
    int 0x80
    STORE 5                             ; no children left: -1

    ; ---- 2. a child that ends by SIGTERM ----
    CHILD 0x40001600, child_term
    STORE 6
    mov eax, SYS_WAIT4
    mov ebx, -1
    mov ecx, STATUS
    xor edx, edx
    xor esi, esi
    int 0x80
    STORE 7
    mov eax, [STATUS]
    STORE 8                             ; 15: killed by SIGTERM

    ; ---- 3. a child that waits for ever; a pidfd ends it ----
    CHILD 0x40001800, child_pause
    STORE 9
    mov eax, SYS_WAIT4
    mov ebx, [KID]
    xor ecx, ecx
    mov edx, WNOHANG
    xor esi, esi
    int 0x80
    STORE 10                            ; still running: 0
    mov eax, SYS_PIDFD_OPEN
    mov ebx, [KID]
    xor ecx, ecx
    int 0x80
    mov [PFD], eax
    STORE 11
    mov eax, SYS_PIDFD_SEND_SIGNAL
    mov ebx, [PFD]
    mov ecx, SIGKILL
    int 0x80
    STORE 12
    mov eax, SYS_WAITID
    mov ebx, P_PIDFD
    mov ecx, [PFD]
    mov edx, INFO
    mov esi, WEXITED
    int 0x80
    STORE 13
    mov eax, [INFO + 8]
    STORE 14                            ; CLD_KILLED
    mov eax, [INFO + 20]
    STORE 15                            ; SIGKILL
    mov eax, SYS_PIDFD_SEND_SIGNAL
    mov ebx, [PFD]
    xor ecx, ecx
    int 0x80
    STORE 16                            ; it is gone: -1
    mov eax, SYS_CLOSE
    mov ebx, [PFD]
    int 0x80
    STORE 17

    ; ---- 4. refusals ----
    mov eax, SYS_WAIT4
    mov ebx, 1                          ; idle: not our child
    xor ecx, ecx
    xor edx, edx
    xor esi, esi
    int 0x80
    STORE 18
    mov eax, SYS_PIDFD_OPEN
    mov ebx, 1                          ; a system process
    xor ecx, ecx
    int 0x80
    STORE 19

    ; ---- 5. usage ----
    mov eax, SYS_GETRUSAGE
    xor ebx, ebx                        ; RUSAGE_SELF
    mov ecx, RUSAGE
    int 0x80
    STORE 20
    mov eax, [RUSAGE + 64]
    STORE 21                            ; voluntary switches: we waited
    mov eax, SYS_GETRUSAGE
    mov ebx, -1                         ; RUSAGE_CHILDREN
    mov ecx, RUSAGE
    int 0x80
    STORE 22
    mov eax, SYS_TIMES
    mov ebx, TMS
    int 0x80
    STORE 23                            ; ticks since boot

    ; ---- 6. pidfd_getfd: a copy of our own open file ----
    mov eax, SYS_OPEN
    mov ebx, ADDR(file_name)
    mov ecx, O_RWC
    int 0x80
    mov [FILEFD], eax
    STORE 24
    mov eax, SYS_WRITE
    mov ebx, [FILEFD]
    mov ecx, ADDR(hi)
    mov edx, 2
    int 0x80
    STORE 25
    mov eax, SYS_PIDFD_OPEN
    mov ebx, [MYPID]
    xor ecx, ecx
    int 0x80
    mov [PFD], eax
    STORE 26
    mov eax, SYS_PIDFD_GETFD
    mov ebx, [PFD]
    mov ecx, [FILEFD]
    xor edx, edx
    int 0x80
    mov [COPYFD], eax
    STORE 27
    mov eax, SYS_SEEK
    mov ebx, [COPYFD]
    xor ecx, ecx
    xor edx, edx                        ; SEEK_SET
    int 0x80
    STORE 28
    mov eax, SYS_READ
    mov ebx, [COPYFD]
    mov ecx, BUF
    mov edx, 2
    int 0x80
    STORE 29
    mov eax, [BUF]
    STORE 30                            ; "hi"
    mov eax, SYS_PIDFD_GETFD
    mov ebx, [PFD]
    mov ecx, 13                         ; not open
    xor edx, edx
    int 0x80
    STORE 31
    mov eax, SYS_CLOSE
    mov ebx, [COPYFD]
    int 0x80
    mov eax, SYS_CLOSE
    mov ebx, [FILEFD]
    int 0x80
    mov eax, SYS_CLOSE
    mov ebx, [PFD]
    int 0x80
    mov eax, SYS_UNLINK
    mov ebx, ADDR(file_name)
    int 0x80

    ; ---- 7. waitpid and wait ----
    CHILD 0x40001A00, child_exit0
    STORE 32
    mov eax, SYS_WAITPID
    mov ebx, [KID]
    int 0x80
    STORE 33
    mov eax, SYS_WAIT
    int 0x80
    STORE 34                            ; none left: -1

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

; ---------- the children ----------
child_exit7:
    mov eax, SYS_EXIT
    mov ebx, 7
    int 0x80
child_term:
    mov eax, SYS_GETPID
    int 0x80
    mov ebx, eax
    mov eax, SYS_KILL
    mov ecx, SIGTERM
    int 0x80
    jmp child_term                      ; never: SIGTERM ends us on the way out
child_pause:
    mov eax, SYS_PAUSE
    int 0x80
    jmp child_pause
child_exit0:
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

file_name: db "/home/guest/pidfd.txt", 0
hi:        db "hi"
wait_program_end:
