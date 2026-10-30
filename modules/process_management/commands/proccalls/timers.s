; =============================================================================
; timers.s - A ring 3 program that sleeps and arms interval timers:
; nanosleep, clock_nanosleep, setitimer, getitimer, alarm, times.
;
; Run by cmd_proccalls.c (group "timers") through usertask, as user 1.
; Each SIGALRM / SIGVTALRM / SIGPROF bumps a counter in SCRATCH; the
; VIRTUAL and PROF timers count only CPU time, so the program spins.
; =============================================================================

SYS_EXIT            equ 1
SYS_RT_SIGACTION    equ 69
SYS_PAUSE           equ 78
SYS_ALARM           equ 79
SYS_TIMES           equ 83
SYS_NANOSLEEP       equ 86
SYS_CLOCK_NANOSLEEP equ 87
SYS_SETITIMER       equ 88
SYS_GETITIMER       equ 89

SIGALRM     equ 14
SIGVTALRM   equ 26
SIGPROF     equ 27
ITIMER_REAL equ 0
ITIMER_VIRT equ 1
ITIMER_PROF equ 2
CLOCK_MONO  equ 1
ABSTIME     equ 1
SPIN_MAX    equ 400000000           ; give up spinning one day

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516

TS      equ SCRATCH + 0         ; timespec (8)
REM     equ SCRATCH + 8         ; timespec (8)
ITV     equ SCRATCH + 16        ; itimerval (16): interval sec/usec, value sec/usec
OLD     equ SCRATCH + 32        ; itimerval (16)
ACT     equ SCRATCH + 48        ; k_sigaction_t (16)
ALRM    equ SCRATCH + 64        ; counters
VT      equ SCRATCH + 68
PROF    equ SCRATCH + 72
T0      equ SCRATCH + 76

%define ADDR(label) (USER_CODE + (label - timers_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; timespec in TS = (sec, nsec)
%macro SETTS 2
    mov dword [TS + 0], %1
    mov dword [TS + 4], %2
%endmacro

; itimerval in ITV = (interval usec, value sec, value usec)
%macro SETITV 3
    mov dword [ITV + 0], 0
    mov dword [ITV + 4], %1
    mov dword [ITV + 8], %2
    mov dword [ITV + 12], %3
%endmacro

; setitimer(which, ITV, OLD) -> slot
%macro SETITIMER 2
    mov eax, SYS_SETITIMER
    mov ebx, %1
    mov ecx, ITV
    mov edx, OLD
    int 0x80
    STORE %2
%endmacro

; sigaction(sig, handler) -> slot
%macro HANDLE 3
    mov dword [ACT + 0], ADDR(%2)
    mov dword [ACT + 4], 0
    mov dword [ACT + 8], 0
    mov dword [ACT + 12], 0
    mov eax, SYS_RT_SIGACTION
    mov ebx, %1
    mov ecx, ACT
    xor edx, edx
    mov esi, 8
    int 0x80
    STORE %3
%endmacro

; spin until the counter at %1 is not 0 (or SPIN_MAX), then store it
%macro SPIN 2
    mov ecx, SPIN_MAX
%%again:
    cmp dword [%1], 0
    jne %%done
    dec ecx
    jnz %%again
%%done:
    mov eax, [%1]
    STORE %2
%endmacro

%macro NANOSLEEP 0
    mov eax, SYS_NANOSLEEP
    mov ebx, TS
    mov ecx, REM
    int 0x80
%endmacro

%macro PAUSE 0
    mov eax, SYS_PAUSE
    int 0x80
%endmacro

global timers_program, timers_program_end

section .text

timers_program:
    mov edi, SCRATCH
    mov ecx, 32
    xor eax, eax
    rep stosd

    ; ---- nanosleep ----
    mov eax, SYS_TIMES
    xor ebx, ebx
    int 0x80
    mov [T0], eax
    SETTS 0, 50000000                   ; 50 ms
    NANOSLEEP
    STORE 1
    mov eax, SYS_TIMES
    xor ebx, ebx
    int 0x80
    sub eax, [T0]
    STORE 2                             ; ticks slept: at least 5
    SETTS 0, 1000000000                 ; nsec = 1 s: invalid
    NANOSLEEP
    STORE 3
    SETTS -1, 0                         ; negative: invalid
    NANOSLEEP
    STORE 4

    ; ---- clock_nanosleep ----
    SETTS 0, 20000000
    mov eax, SYS_CLOCK_NANOSLEEP
    mov ebx, CLOCK_MONO
    xor ecx, ecx
    mov edx, TS
    xor esi, esi
    int 0x80
    STORE 5
    SETTS 0, 0                          ; boot time: long past
    mov eax, SYS_CLOCK_NANOSLEEP
    mov ebx, CLOCK_MONO
    mov ecx, ABSTIME
    mov edx, TS
    xor esi, esi
    int 0x80
    STORE 6
    mov eax, SYS_CLOCK_NANOSLEEP
    mov ebx, 99                         ; no such clock
    xor ecx, ecx
    mov edx, TS
    xor esi, esi
    int 0x80
    STORE 7                             ; EINVAL = 22

    ; ---- ITIMER_REAL: once, then every 20 ms ----
    HANDLE SIGALRM, h_alrm, 8
    SETITV 0, 0, 30000
    SETITIMER ITIMER_REAL, 9
    PAUSE
    STORE 10
    mov eax, [ALRM]
    STORE 11
    SETITV 20000, 0, 20000
    SETITIMER ITIMER_REAL, 12
    PAUSE
    PAUSE
    PAUSE
    mov eax, [ALRM]
    STORE 13                            ; 1 + 3
    mov eax, SYS_GETITIMER
    mov ebx, ITIMER_REAL
    mov ecx, OLD
    int 0x80
    STORE 14
    mov eax, [OLD + 4]
    STORE 15                            ; interval: 20000 us
    SETITV 0, 0, 0                      ; disarm
    SETITIMER ITIMER_REAL, 16
    mov eax, [OLD + 4]
    STORE 17

    ; ---- CPU-time timers ----
    HANDLE SIGVTALRM, h_vt, 18
    SETITV 0, 0, 50000
    SETITIMER ITIMER_VIRT, 19
    SPIN VT, 20
    HANDLE SIGPROF, h_prof, 21
    SETITIMER ITIMER_PROF, 22
    SPIN PROF, 23

    ; ---- refusals ----
    SETITIMER 5, 24                     ; no such timer
    SETITV 0, -1, 0
    SETITIMER ITIMER_REAL, 25           ; negative: sleep_bounds

    ; ---- a signal cuts a sleep short ----
    SETITV 0, 0, 30000
    SETITIMER ITIMER_REAL, 26
    SETTS 1, 0
    NANOSLEEP
    STORE 27                            ; EINTR: -1
    mov eax, [REM + 4]
    STORE 28                            ; ~970 ms were left
    mov eax, [ALRM]
    STORE 29

    ; ---- alarm() is ITIMER_REAL ----
    mov eax, SYS_ALARM
    mov ebx, 1
    int 0x80
    STORE 30
    mov eax, SYS_GETITIMER
    mov ebx, ITIMER_REAL
    mov ecx, OLD
    int 0x80
    mov eax, [OLD + 8]
    STORE 31                            ; 1 s left (or just under: 0)
    mov eax, SYS_ALARM
    xor ebx, ebx
    int 0x80
    STORE 32                            ; 1 s was left

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

h_alrm:
    inc dword [ALRM]
    ret
h_vt:
    inc dword [VT]
    ret
h_prof:
    inc dword [PROF]
    ret
timers_program_end:
