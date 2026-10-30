; =============================================================================
; signals.s - A ring 3 program that calls every signal syscall of
; process_management and checks what really happened.
;
; Run by cmd_proccalls.c through usertask, as user 1 (guest). Each answer
; (EAX, or what a handler saw) goes into one mailbox slot; the kernel
; prints them all after the program ends.
;
; Memory (the two usertask pages, see usertask.h):
;   0x40000F00  MAILBOX    64 words of answers
;   0x40001000  SCRATCH    structs passed to the syscalls, handler notes
;   0x40001400  ALTSTACK   1 KiB alternate signal stack (sigaltstack)
;   0x40002000  the program's own stack top
;
; Numbers match modules/process_management/syscalls/<name>/sys_<name>.h.
; =============================================================================

SYS_EXIT              equ 1
SYS_GETPID            equ 2
SYS_KILL              equ 58
SYS_RT_SIGACTION      equ 69
SYS_RT_SIGPROCMASK    equ 70
SYS_RT_SIGPENDING     equ 71
SYS_RT_SIGSUSPEND     equ 72
SYS_RT_SIGTIMEDWAIT   equ 73
SYS_RT_SIGQUEUEINFO   equ 74
SYS_RT_TGSIGQUEUEINFO equ 75
SYS_RT_SIGRETURN      equ 76
SYS_SIGALTSTACK       equ 77
SYS_PAUSE             equ 78
SYS_ALARM             equ 79

SIGKILL     equ 9
SIGUSR1     equ 10
SIGUSR2     equ 12
SIGALRM     equ 14
SIG_IGN     equ 1
SIG_BLOCK   equ 0
SIG_UNBLOCK equ 1
SA_SIGINFO  equ 0x00000004
SA_RESTORER equ 0x04000000
SA_ONSTACK  equ 0x08000000

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
ALTSTACK    equ 0x40001400
ALTSIZE     equ 1024
DONE        equ 0x516

; scratch layout
ACT     equ SCRATCH + 0         ; k_sigaction_t (16)
OACT    equ SCRATCH + 16        ; k_sigaction_t (16)
SET     equ SCRATCH + 32        ; sigset (8)
OSET    equ SCRATCH + 40        ; sigset (8)
INFO    equ SCRATCH + 48        ; k_siginfo_t (24)
TS      equ SCRATCH + 72        ; timespec (8)
NEWSS   equ SCRATCH + 80        ; k_stack_t (12)
OSS     equ SCRATCH + 92        ; k_stack_t (12)
COUNT   equ SCRATCH + 104       ; h1: how often it ran
ARG     equ SCRATCH + 108       ; h1: its argument
H2ESP   equ SCRATCH + 112       ; h2: its stack pointer
SIGNO2  equ SCRATCH + 116       ; h2: info->signo
CODE2   equ SCRATCH + 120       ; h2: info->code
PID2    equ SCRATCH + 124       ; h2: info->pid
VALUE2  equ SCRATCH + 128       ; h2: info->value
COUNT2  equ SCRATCH + 132       ; h2: how often it ran
ALRM    equ SCRATCH + 136       ; h3: how often SIGALRM came
MYPID   equ SCRATCH + 140

%define ADDR(label) (USER_CODE + (label - signals_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; sigaction(sig, handler, flags, restorer) -> eax
%macro SIGACTION 4
    mov dword [ACT + 0], %2
    mov dword [ACT + 4], %3
    mov dword [ACT + 8], %4
    mov dword [ACT + 12], 0
    mov eax, SYS_RT_SIGACTION
    mov ebx, %1
    mov ecx, ACT
    mov edx, OACT
    mov esi, 8
    int 0x80
%endmacro

; sigprocmask(how, set) -> eax, old mask in OSET
%macro SIGPROCMASK 2
    mov dword [SET], %2
    mov dword [SET + 4], 0
    mov eax, SYS_RT_SIGPROCMASK
    mov ebx, %1
    mov ecx, SET
    mov edx, OSET
    mov esi, 8
    int 0x80
%endmacro

; kill(me, sig) -> eax
%macro KILLME 1
    mov eax, SYS_KILL
    mov ebx, [MYPID]
    mov ecx, %1
    int 0x80
%endmacro

global signals_program, signals_program_end

section .text

signals_program:
    mov edi, SCRATCH                    ; the page is reused: forget old notes
    mov ecx, 64
    xor eax, eax
    rep stosd
    mov eax, SYS_GETPID
    int 0x80
    mov [MYPID], eax

    ; ---- a handler runs, through the kernel's trampoline ----
    SIGACTION SIGUSR1, ADDR(h1), 0, 0
    STORE 1
    mov eax, [OACT]
    STORE 2                             ; the old handler: SIG_DFL
    KILLME SIGUSR1
    STORE 3
    mov eax, [COUNT]
    STORE 4                             ; h1 ran once
    mov eax, [ARG]
    STORE 5                             ; ...with argument 10

    ; ---- blocked: it waits ----
    SIGPROCMASK SIG_BLOCK, 1 << SIGUSR1
    STORE 6
    mov eax, [OSET]
    STORE 7
    KILLME SIGUSR1
    STORE 8
    mov eax, [COUNT]
    STORE 9                             ; still 1
    mov eax, SYS_RT_SIGPENDING
    mov ebx, SET
    mov ecx, 8
    int 0x80
    STORE 10
    mov eax, [SET]
    STORE 11                            ; USR1 is pending
    SIGPROCMASK SIG_UNBLOCK, 1 << SIGUSR1
    STORE 12
    mov eax, [COUNT]
    STORE 13                            ; delivered on the way out: 2

    ; ---- handler_bounds ----
    SIGACTION SIGKILL, ADDR(h1), 0, 0
    STORE 14                            ; refused
    SIGACTION SIGUSR2, 0x00100000, 0, 0
    STORE 15                            ; a kernel address: refused

    ; ---- SIG_IGN throws it away ----
    SIGACTION SIGUSR2, SIG_IGN, 0, 0
    STORE 16
    KILLME SIGUSR2
    STORE 17
    mov eax, SYS_RT_SIGPENDING
    mov ebx, SET
    mov ecx, 8
    int 0x80
    mov eax, [SET]
    STORE 18                            ; nothing pending

    ; ---- sigaltstack + SA_SIGINFO + SA_RESTORER ----
    mov dword [NEWSS + 0], ALTSTACK
    mov dword [NEWSS + 4], 0
    mov dword [NEWSS + 8], ALTSIZE
    mov eax, SYS_SIGALTSTACK
    mov ebx, NEWSS
    mov ecx, OSS
    int 0x80
    STORE 19
    mov eax, [OSS + 4]
    STORE 20                            ; there was none: SS_DISABLE
    SIGACTION SIGUSR1, ADDR(h2), SA_SIGINFO | SA_ONSTACK | SA_RESTORER, ADDR(restorer)
    STORE 21
    KILLME SIGUSR1
    STORE 22
    mov eax, [H2ESP]                    ; did h2 run on the alternate stack?
    sub eax, ALTSTACK
    cmp eax, ALTSIZE
    setb al
    movzx eax, al
    STORE 23
    mov eax, [SIGNO2]
    STORE 24
    mov eax, [CODE2]
    STORE 25                            ; SI_USER
    mov eax, [PID2]
    STORE 26                            ; sent by us

    ; ---- queued signals carry a value ----
    mov dword [INFO + 20], 1234
    mov eax, SYS_RT_SIGQUEUEINFO
    mov ebx, [MYPID]
    mov ecx, SIGUSR1
    mov edx, INFO
    int 0x80
    STORE 27
    mov eax, [VALUE2]
    STORE 28
    mov eax, [CODE2]
    STORE 29                            ; SI_QUEUE
    mov dword [INFO + 20], 77
    mov eax, SYS_RT_TGSIGQUEUEINFO
    mov ebx, [MYPID]                    ; tgid: we are the group's first thread
    mov ecx, [MYPID]
    mov edx, SIGUSR1
    mov esi, INFO
    int 0x80
    STORE 30
    mov eax, [VALUE2]
    STORE 31
    mov eax, [COUNT2]
    STORE 32                            ; h2 ran 3 times

    ; ---- alarm + pause ----
    SIGACTION SIGALRM, ADDR(h3), 0, 0
    STORE 33
    mov eax, SYS_ALARM
    mov ebx, 2
    int 0x80
    STORE 34                            ; no alarm before: 0
    mov eax, SYS_ALARM
    mov ebx, 1
    int 0x80
    STORE 35                            ; 2 seconds were left
    mov eax, SYS_PAUSE
    int 0x80
    STORE 36                            ; -1 after the handler
    mov eax, [ALRM]
    STORE 37

    ; ---- rt_sigtimedwait takes a blocked signal ----
    SIGACTION SIGUSR2, 0, 0, 0          ; back to SIG_DFL
    STORE 38
    SIGPROCMASK SIG_BLOCK, 1 << SIGUSR2
    STORE 39
    KILLME SIGUSR2
    STORE 40
    mov dword [SET], 1 << SIGUSR2
    mov eax, SYS_RT_SIGTIMEDWAIT
    mov ebx, SET
    mov ecx, INFO
    xor edx, edx                        ; wait for ever
    mov esi, 8
    int 0x80
    STORE 41
    mov eax, [INFO]
    STORE 42
    mov dword [TS + 0], 0
    mov dword [TS + 4], 50000000        ; 50 ms
    mov eax, SYS_RT_SIGTIMEDWAIT
    mov ebx, SET
    mov ecx, INFO
    mov edx, TS
    mov esi, 8
    int 0x80
    STORE 43                            ; nothing came: -1

    ; ---- rt_sigsuspend: another mask while waiting ----
    mov eax, SYS_ALARM
    mov ebx, 1
    int 0x80
    STORE 44
    mov dword [SET], 0
    mov eax, SYS_RT_SIGSUSPEND
    mov ebx, SET
    mov ecx, 8
    int 0x80
    STORE 45
    mov eax, [ALRM]
    STORE 46
    SIGPROCMASK SIG_BLOCK, 0
    STORE 47
    mov eax, [OSET]
    STORE 48                            ; the old mask is back: USR2

    ; ---- done; last, a forged rt_sigreturn must end us ----
    mov dword [MAILBOX], DONE
    mov eax, SYS_RT_SIGRETURN
    int 0x80
    mov dword [MAILBOX + 49 * 4], 0xBAD ; never: sigframe_check ends us
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

; handler(sig): count, remember the argument
h1:
    inc dword [COUNT]
    mov eax, [esp + 4]
    mov [ARG], eax
    ret

; handler(sig, info, context): where it runs and what info says
h2:
    mov [H2ESP], esp
    mov eax, [esp + 8]                  ; info
    mov ecx, [eax + 0]
    mov [SIGNO2], ecx
    mov ecx, [eax + 8]
    mov [CODE2], ecx
    mov ecx, [eax + 12]
    mov [PID2], ecx
    mov ecx, [eax + 20]
    mov [VALUE2], ecx
    inc dword [COUNT2]
    ret

; SIGALRM
h3:
    inc dword [ALRM]
    ret

; SA_RESTORER: what libc's __restore_rt does
restorer:
    mov eax, SYS_RT_SIGRETURN
    int 0x80
signals_program_end:
