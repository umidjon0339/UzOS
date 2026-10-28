; =============================================================================
; cpucalls.s - A ring 3 program that calls every CPU-management syscall.
;
; Run by cmd_cpucalls.c through usertask (process_management), as user 1
; (guest), so the permission checks are the ones a normal user meets.
; Each answer (EAX, or a value the kernel wrote into memory) goes into one
; mailbox slot; the kernel prints them all after the program ends.
;
; Memory it uses (all in the two usertask pages, see usertask.h):
;   0x40000F00  MAILBOX   64 words of answers   (end of the code page)
;   0x40001000  SCRATCH   structs passed to the syscalls
;   0x40001400  stack top of the clone3() thread
;   0x40001800  stack top of the clone() thread
;   0x40002000  the program's own stack top
;
; Numbers match modules/cpu_management/syscalls/<name>/sys_<name>.h.
; =============================================================================

SYS_EXIT                   equ 1
SYS_GETPID                 equ 2
SYS_SLEEP                  equ 3
SYS_SCHED_YIELD            equ 4
SYS_READ                   equ 15
SYS_CLOSE                  equ 17
SYS_SETPRIORITY            equ 21
SYS_GETPRIORITY            equ 22
SYS_SCHED_SETSCHEDULER     equ 26
SYS_SCHED_GETSCHEDULER     equ 27
SYS_SCHED_SETPARAM         equ 28
SYS_SCHED_GETPARAM         equ 29
SYS_SCHED_GET_PRIORITY_MIN equ 30
SYS_SCHED_GET_PRIORITY_MAX equ 31
SYS_SCHED_RR_GET_INTERVAL  equ 32
SYS_SCHED_SETAFFINITY      equ 33
SYS_SCHED_GETAFFINITY      equ 34
SYS_SCHED_SETATTR          equ 35
SYS_SCHED_GETATTR          equ 36
SYS_GETRLIMIT              equ 37
SYS_SETRLIMIT              equ 38
SYS_PRLIMIT64              equ 39
SYS_GETCPU                 equ 40
SYS_PERF_EVENT_OPEN        equ 41
SYS_GETTID                 equ 42
SYS_CLONE                  equ 43
SYS_CLONE3                 equ 44
SYS_MOVE_PAGES             equ 45
SYS_MBIND                  equ 46
SYS_SET_MEMPOLICY          equ 47
SYS_GET_MEMPOLICY          equ 48
SYS_MIGRATE_PAGES          equ 49
SYS_MEMBARRIER             equ 50
SYS_EXIT_GROUP             equ 51

MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0xCA11

; scratch layout
PARAM       equ SCRATCH + 0     ; struct sched_param
TIMESPEC    equ SCRATCH + 4     ; struct timespec (8)
MASK        equ SCRATCH + 12    ; cpu mask (4)
ATTR        equ SCRATCH + 16    ; struct sched_attr (48)
RLIM        equ SCRATCH + 64    ; struct rlimit (8)
RLIM64      equ SCRATCH + 72    ; struct rlimit64 (16)
CPU_OUT     equ SCRATCH + 88
NODE_OUT    equ SCRATCH + 92
PERF_ATTR   equ SCRATCH + 96    ; perf_event_attr (16)
COUNT       equ SCRATCH + 112   ; a 64-bit counter
NODEMASK    equ SCRATCH + 120
MODE        equ SCRATCH + 124
PAGES       equ SCRATCH + 128   ; two addresses
STATUS      equ SCRATCH + 136   ; two answers
PTID        equ SCRATCH + 144   ; clone() writes the child's id here
OLD_NODES   equ SCRATCH + 148
NEW_NODES   equ SCRATCH + 152
FLAG        equ SCRATCH + 156   ; the threads say "done" here
ARGS3       equ SCRATCH + 160   ; struct clone_args (64)
PTID3       equ SCRATCH + 224

CLONE_THREAD_FLAGS equ 0x00010900   ; CLONE_VM | CLONE_SIGHAND | CLONE_THREAD
CLONE_PARENT_SETTID equ 0x00100000

global cpucalls_program, cpucalls_program_end

section .text

; SAVE slot: mailbox[slot] = eax
%macro SAVE 1
    mov [MAILBOX + 4 * %1], eax
%endmacro

; COPY slot, address: mailbox[slot] = the dword at address
%macro COPY 2
    mov eax, [%2]
    mov [MAILBOX + 4 * %1], eax
%endmacro

cpucalls_program:
    ; --- who am I --------------------------------------------------------
    mov eax, SYS_GETPID
    int 0x80
    SAVE 1
    mov eax, SYS_GETTID
    int 0x80
    SAVE 2
    mov eax, SYS_SCHED_YIELD
    int 0x80
    SAVE 3

    ; --- nice: getpriority / setpriority ---------------------------------
    mov eax, SYS_GETPRIORITY
    mov ebx, 0                  ; PRIO_PROCESS
    mov ecx, 0                  ; me
    int 0x80
    SAVE 4                      ; 20 - nice = 20
    mov eax, SYS_SETPRIORITY
    mov ebx, 0
    mov ecx, 0
    mov edx, 5                  ; nicer: allowed for anyone
    int 0x80
    SAVE 5
    mov eax, SYS_GETPRIORITY
    mov ebx, 0
    mov ecx, 0
    int 0x80
    SAVE 6                      ; 20 - 5 = 15
    mov eax, SYS_SETPRIORITY
    mov ebx, 0
    mov ecx, 0
    mov edx, -5                 ; greedier: root only
    int 0x80
    SAVE 7

    ; --- policies -------------------------------------------------------
    mov eax, SYS_SCHED_GETSCHEDULER
    mov ebx, 0
    int 0x80
    SAVE 8                      ; SCHED_OTHER = 0
    mov dword [PARAM], 0
    mov eax, SYS_SCHED_SETSCHEDULER
    mov ebx, 0
    mov ecx, 3                  ; SCHED_BATCH
    mov edx, PARAM
    int 0x80
    SAVE 9
    mov eax, SYS_SCHED_GETSCHEDULER
    mov ebx, 0
    int 0x80
    SAVE 10                     ; 3
    mov dword [PARAM], 10
    mov eax, SYS_SCHED_SETSCHEDULER
    mov ebx, 0
    mov ecx, 1                  ; SCHED_FIFO 10: needs root or RLIMIT_RTPRIO
    mov edx, PARAM
    int 0x80
    SAVE 11
    mov dword [PARAM], 77
    mov eax, SYS_SCHED_GETPARAM
    mov ebx, 0
    mov ecx, PARAM
    int 0x80
    SAVE 12
    COPY 13, PARAM              ; 0: batch has no rt priority
    mov dword [PARAM], 5
    mov eax, SYS_SCHED_SETPARAM
    mov ebx, 0
    mov ecx, PARAM              ; priority 5 under SCHED_BATCH: out of range
    int 0x80
    SAVE 14
    mov eax, SYS_SCHED_GET_PRIORITY_MIN
    mov ebx, 1                  ; SCHED_FIFO
    int 0x80
    SAVE 15
    mov eax, SYS_SCHED_GET_PRIORITY_MAX
    mov ebx, 1
    int 0x80
    SAVE 16
    mov eax, SYS_SCHED_RR_GET_INTERVAL
    mov ebx, 0
    mov ecx, TIMESPEC
    int 0x80
    SAVE 17
    mov eax, [TIMESPEC + 4]     ; nanoseconds -> milliseconds
    xor edx, edx
    mov ecx, 1000000
    div ecx
    SAVE 18

    ; --- affinity -------------------------------------------------------
    mov eax, SYS_SCHED_GETAFFINITY
    mov ebx, 0
    mov ecx, 4
    mov edx, MASK
    int 0x80
    SAVE 19                     ; 4 bytes written
    COPY 20, MASK               ; 1 = CPU 0
    mov dword [MASK], 2         ; CPU 1 only: there is no CPU 1
    mov eax, SYS_SCHED_SETAFFINITY
    mov ebx, 0
    mov ecx, 4
    mov edx, MASK
    int 0x80
    SAVE 21
    mov dword [MASK], 1
    mov eax, SYS_SCHED_SETAFFINITY
    mov ebx, 0
    mov ecx, 4
    mov edx, MASK
    int 0x80
    SAVE 22

    ; --- sched_attr -----------------------------------------------------
    mov eax, SYS_SCHED_GETATTR
    mov ebx, 0
    mov ecx, ATTR
    mov edx, 48
    mov esi, 0
    int 0x80
    SAVE 23
    COPY 24, ATTR + 4           ; sched_policy: 3 (batch)
    COPY 25, ATTR + 16          ; sched_nice: 5
    mov dword [ATTR + 4], 0     ; SCHED_OTHER
    mov dword [ATTR + 16], 7    ; nice 7
    mov eax, SYS_SCHED_SETATTR
    mov ebx, 0
    mov ecx, ATTR
    mov edx, 0
    int 0x80
    SAVE 26
    mov eax, SYS_GETPRIORITY
    mov ebx, 0
    mov ecx, 0
    int 0x80
    SAVE 27                     ; 20 - 7 = 13

    ; --- resource limits ------------------------------------------------
    mov eax, SYS_GETRLIMIT
    mov ebx, 7                  ; RLIMIT_NOFILE
    mov ecx, RLIM
    int 0x80
    SAVE 28
    COPY 29, RLIM               ; soft: 16
    mov dword [RLIM], 8
    mov dword [RLIM + 4], 8
    mov eax, SYS_SETRLIMIT      ; lower both to 8: allowed
    mov ebx, 7
    mov ecx, RLIM
    int 0x80
    SAVE 30
    mov dword [RLIM], 16
    mov dword [RLIM + 4], 16
    mov eax, SYS_SETRLIMIT      ; back up to 16: a raise needs root
    mov ebx, 7
    mov ecx, RLIM
    int 0x80
    SAVE 31
    mov eax, SYS_PRLIMIT64
    mov ebx, 0
    mov ecx, 7
    mov edx, 0                  ; nothing new...
    mov esi, RLIM64             ; ...just read the old one
    int 0x80
    SAVE 32
    COPY 33, RLIM64             ; low half of cur: 8

    ; --- where am I running ---------------------------------------------
    mov eax, SYS_GETCPU
    mov ebx, CPU_OUT
    mov ecx, NODE_OUT
    int 0x80
    SAVE 34
    COPY 35, CPU_OUT
    COPY 36, NODE_OUT

    ; --- perf: count my own context switches -----------------------------
    mov dword [PERF_ATTR], 1        ; PERF_TYPE_SOFTWARE
    mov dword [PERF_ATTR + 4], 16   ; size
    mov dword [PERF_ATTR + 8], 3    ; PERF_COUNT_SW_CONTEXT_SWITCHES
    mov dword [PERF_ATTR + 12], 0
    mov eax, SYS_PERF_EVENT_OPEN
    mov ebx, PERF_ATTR
    mov ecx, 0                  ; me
    mov edx, -1                 ; any CPU
    mov esi, -1                 ; no group
    mov edi, 0
    int 0x80
    SAVE 37                     ; a file descriptor
    mov ebp, eax
    mov eax, SYS_SLEEP          ; two sleeps = two switches away from me
    mov ebx, 20
    int 0x80
    mov eax, SYS_SLEEP
    mov ebx, 20
    int 0x80
    mov eax, SYS_READ
    mov ebx, ebp
    mov ecx, COUNT
    mov edx, 8
    int 0x80
    SAVE 38                     ; 8 bytes
    COPY 39, COUNT              ; >= 2
    mov eax, SYS_CLOSE
    mov ebx, ebp
    int 0x80
    SAVE 40
    mov dword [PERF_ATTR], 0        ; PERF_TYPE_HARDWARE
    mov dword [PERF_ATTR + 8], 1    ; INSTRUCTIONS: needs a PMU
    mov eax, SYS_PERF_EVENT_OPEN
    mov ebx, PERF_ATTR
    mov ecx, 0
    mov edx, -1
    mov esi, -1
    mov edi, 0
    int 0x80
    SAVE 41

    ; --- NUMA -----------------------------------------------------------
    mov dword [NODEMASK], 1     ; node 0
    mov eax, SYS_SET_MEMPOLICY
    mov ebx, 2                  ; MPOL_BIND
    mov ecx, NODEMASK
    mov edx, 32
    int 0x80
    SAVE 42
    mov eax, SYS_GET_MEMPOLICY
    mov ebx, MODE
    mov ecx, NODEMASK
    mov edx, 32
    mov esi, 0
    mov edi, 0
    int 0x80
    SAVE 43
    COPY 44, MODE               ; 2
    mov dword [NODEMASK], 2     ; node 1: does not exist
    mov eax, SYS_SET_MEMPOLICY
    mov ebx, 2
    mov ecx, NODEMASK
    mov edx, 32
    int 0x80
    SAVE 45
    mov dword [NODEMASK], 1
    mov eax, SYS_MBIND          ; the stack page: MPOL_PREFERRED node 0
    mov ebx, 0x40001000
    mov ecx, 4096
    mov edx, 1
    mov esi, NODEMASK
    mov edi, 32
    mov ebp, 0
    int 0x80
    SAVE 46
    mov eax, SYS_GET_MEMPOLICY  ; the policy at that address: 1
    mov ebx, MODE
    mov ecx, 0
    mov edx, 0
    mov esi, 0x40001000
    mov edi, 2                  ; MPOL_F_ADDR
    int 0x80
    COPY 47, MODE
    mov eax, SYS_GET_MEMPOLICY  ; the node of that page: 0
    mov ebx, MODE
    mov ecx, 0
    mov edx, 0
    mov esi, 0x40001000
    mov edi, 3                  ; MPOL_F_NODE | MPOL_F_ADDR
    int 0x80
    SAVE 48
    COPY 49, MODE
    mov dword [PAGES], 0x40000000       ; our code page
    mov dword [PAGES + 4], 0x7FFFF000   ; nothing mapped there
    mov eax, SYS_MOVE_PAGES
    mov ebx, 0
    mov ecx, 2
    mov edx, PAGES
    mov esi, 0                  ; no target nodes: just ask
    mov edi, STATUS
    mov ebp, 0
    int 0x80
    SAVE 50
    COPY 51, STATUS             ; 0: node 0
    COPY 52, STATUS + 4         ; -2: ENOENT, not mapped
    mov dword [OLD_NODES], 1
    mov dword [NEW_NODES], 1
    mov eax, SYS_MIGRATE_PAGES
    mov ebx, 0
    mov ecx, 32
    mov edx, OLD_NODES
    mov esi, NEW_NODES
    int 0x80
    SAVE 53

    ; --- membarrier ------------------------------------------------------
    mov eax, SYS_MEMBARRIER
    mov ebx, 0                  ; QUERY
    mov ecx, 0
    int 0x80
    SAVE 54
    mov eax, SYS_MEMBARRIER
    mov ebx, 8                  ; PRIVATE_EXPEDITED before registering
    mov ecx, 0
    int 0x80
    SAVE 55
    mov eax, SYS_MEMBARRIER
    mov ebx, 16                 ; REGISTER_PRIVATE_EXPEDITED
    mov ecx, 0
    int 0x80
    SAVE 56
    mov eax, SYS_MEMBARRIER
    mov ebx, 8
    mov ecx, 0
    int 0x80
    SAVE 57

    ; --- clone(): a thread that shares this memory ----------------------
    mov dword [FLAG], 0
    mov eax, SYS_CLONE
    mov ebx, CLONE_THREAD_FLAGS | CLONE_PARENT_SETTID
    mov ecx, 0x40001800         ; its stack
    mov edx, PTID
    mov esi, 0
    mov edi, 0
    int 0x80
    test eax, eax
    jz .thread1                 ; eax 0: we are the new thread
    SAVE 58                     ; parent: the thread's id
.wait1:
    cmp dword [FLAG], 1
    je .joined1
    mov eax, SYS_SLEEP
    mov ebx, 10
    int 0x80
    jmp .wait1
.joined1:
    COPY 59, PTID               ; CLONE_PARENT_SETTID wrote it

    ; clone() without CLONE_VM would need a copied address space: refused
    mov eax, SYS_CLONE
    mov ebx, 0x11               ; SIGCHLD only
    mov ecx, 0x40001800
    mov edx, 0
    mov esi, 0
    mov edi, 0
    int 0x80
    SAVE 62

    ; --- clone3(): the same, with a struct ------------------------------
    mov edi, ARGS3
    mov ecx, 16
    xor eax, eax
    rep stosd                   ; struct clone_args = { 0 }
    mov dword [ARGS3], CLONE_THREAD_FLAGS | CLONE_PARENT_SETTID   ; flags
    mov dword [ARGS3 + 24], PTID3       ; parent_tid
    mov dword [ARGS3 + 40], 0x40001200  ; stack (lowest address)
    mov dword [ARGS3 + 48], 0x200       ; stack_size
    mov dword [FLAG], 0
    mov eax, SYS_CLONE3
    mov ebx, ARGS3
    mov ecx, 64
    int 0x80
    test eax, eax
    jz .thread2
    SAVE 63
.wait2:
    cmp dword [FLAG], 2
    je .joined2
    mov eax, SYS_SLEEP
    mov ebx, 10
    int 0x80
    jmp .wait2
.joined2:

    ; --- the end: every thread of the group -----------------------------
    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT_GROUP
    mov ebx, 0
    int 0x80

; The clone() thread: report who it is, then end ONLY itself.
.thread1:
    mov eax, SYS_GETTID
    int 0x80
    SAVE 60                     ; its own id
    mov eax, SYS_GETPID
    int 0x80
    SAVE 61                     ; the group's id: the parent's pid
    mov dword [FLAG], 1
    mov eax, SYS_EXIT
    int 0x80

; The clone3() thread.
.thread2:
    mov dword [FLAG], 2
    mov eax, SYS_EXIT
    int 0x80
cpucalls_program_end:
