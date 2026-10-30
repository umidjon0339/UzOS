; =============================================================================
; local.s - A ring 3 program for AF_UNIX sockets and for waiting on many
; descriptors: socketpair, SO_PEERCRED, a named UNIX server, poll,
; select, epoll_create1 / epoll_ctl / epoll_wait, ioctl FIONREAD / FIONBIO.
;
; Run by cmd_netcalls.c (group "local") through usertask, as user 1 (guest).
; =============================================================================

SYS_EXIT          equ 1
SYS_READ          equ 15
SYS_WRITE         equ 16
SYS_CLOSE         equ 17
SYS_SOCKET        equ 133
SYS_BIND          equ 134
SYS_CONNECT       equ 135
SYS_LISTEN        equ 136
SYS_ACCEPT        equ 137
SYS_SOCKETPAIR    equ 141
SYS_GETSOCKOPT    equ 148
SYS_SELECT        equ 152
SYS_POLL          equ 154
SYS_EPOLL_CREATE1 equ 157
SYS_EPOLL_CTL     equ 158
SYS_EPOLL_WAIT    equ 159
SYS_IOCTL         equ 165

AF_UNIX       equ 1
SOCK_STREAM   equ 1
SOL_SOCKET    equ 1
SO_PEERCRED   equ 17
POLLIN        equ 1
EPOLL_CTL_ADD equ 1
FIONREAD      equ 0x541B
FIONBIO       equ 0x5421

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516

SV      equ SCRATCH + 0             ; socketpair's two descriptors (8)
CRED    equ SCRATCH + 8             ; pid, uid, gid (12)
LEN     equ SCRATCH + 20
PFD     equ SCRATCH + 24            ; pollfd: fd (4), events (2), revents (2)
SET     equ SCRATCH + 32            ; a select bit set (4)
EV      equ SCRATCH + 36            ; epoll_event: events (4), data (8)
VALUE   equ SCRATCH + 48
EPFD    equ SCRATCH + 52
FD_L    equ SCRATCH + 56
FD_C    equ SCRATCH + 60
UNADDR  equ SCRATCH + 64            ; sockaddr_un: family (2), path
BUF     equ SCRATCH + 192           ; 64 bytes

%define ADDR(label) (USER_CODE + (label - local_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

%macro CALL5 6
    mov eax, %1
    mov ebx, %2
    mov ecx, %3
    mov edx, %4
    mov esi, %5
    mov edi, %6
    int 0x80
%endmacro

global local_program, local_program_end

section .text
local_program:
    ; 1-4: a connected pair; bytes go through; the kernel knows who is on the other end
    CALL5 SYS_SOCKETPAIR, AF_UNIX, SOCK_STREAM, 0, SV, 0
    STORE 1                             ; 0
    CALL5 SYS_WRITE, [SV + 0], ADDR(hello), 5, 0, 0
    STORE 2                             ; 5
    CALL5 SYS_READ, [SV + 4], BUF, 64, 0, 0
    STORE 3                             ; 5
    mov dword [LEN], 12
    CALL5 SYS_GETSOCKOPT, [SV + 4], SOL_SOCKET, SO_PEERCRED, CRED, LEN
    mov eax, [CRED + 4]
    STORE 4                             ; uid 1: guest, as the kernel recorded it

    ; 5-7: poll: nothing to read yet; then write, and it is ready
    mov eax, [SV + 4]
    mov [PFD + 0], eax
    mov word [PFD + 4], POLLIN
    mov word [PFD + 6], 0
    CALL5 SYS_POLL, PFD, 1, 0, 0, 0     ; timeout 0: only look
    STORE 5                             ; 0 ready
    CALL5 SYS_WRITE, [SV + 0], ADDR(hello), 5, 0, 0
    CALL5 SYS_POLL, PFD, 1, 1000, 0, 0
    STORE 6                             ; 1 ready
    movzx eax, word [PFD + 6]
    STORE 7                             ; POLLIN (1)

    ; 8-9: select sees the same; ioctl FIONREAD says how much
    mov ecx, [SV + 4]
    mov eax, 1
    shl eax, cl
    mov [SET], eax
    mov ebx, [SV + 4]
    inc ebx
    CALL5 SYS_SELECT, ebx, SET, 0, 0, 0
    STORE 8                             ; 1
    CALL5 SYS_IOCTL, [SV + 4], FIONREAD, VALUE, 0, 0
    mov eax, [VALUE]
    STORE 9                             ; 5 bytes waiting

    ; 10-13: epoll: a list in the kernel; it reports the ready one with our data
    CALL5 SYS_EPOLL_CREATE1, 0, 0, 0, 0, 0
    mov [EPFD], eax
    STORE 10                            ; a descriptor
    mov dword [EV + 0], POLLIN
    mov dword [EV + 4], 0x1234          ; our data, given back by epoll_wait
    mov dword [EV + 8], 0
    CALL5 SYS_EPOLL_CTL, [EPFD], EPOLL_CTL_ADD, [SV + 4], EV, 0
    STORE 11                            ; 0
    mov dword [EV + 4], 0
    CALL5 SYS_EPOLL_WAIT, [EPFD], EV, 1, 1000, 0
    STORE 12                            ; 1 event
    mov eax, [EV + 4]
    STORE 13                            ; 0x1234

    ; 14-16: read it all; then FIONBIO (non-blocking): an empty read says EAGAIN
    CALL5 SYS_READ, [SV + 4], BUF, 64, 0, 0
    STORE 14                            ; 5
    mov dword [VALUE], 1
    CALL5 SYS_IOCTL, [SV + 4], FIONBIO, VALUE, 0, 0
    STORE 15                            ; 0
    CALL5 SYS_READ, [SV + 4], BUF, 64, 0, 0
    STORE 16                            ; -11 EAGAIN

    ; 17-20: a named server: bind "/tmp/netcalls", listen, connect, accept
    CALL5 SYS_SOCKET, AF_UNIX, SOCK_STREAM, 0, 0, 0
    mov [FD_L], eax
    mov word [UNADDR], AF_UNIX
    mov esi, ADDR(path)
    lea edi, [UNADDR + 2]
    mov ecx, 14
    rep movsb
    CALL5 SYS_BIND, [FD_L], UNADDR, 16, 0, 0
    STORE 17                            ; 0
    CALL5 SYS_LISTEN, [FD_L], 2, 0, 0, 0
    STORE 18                            ; 0
    CALL5 SYS_SOCKET, AF_UNIX, SOCK_STREAM, 0, 0, 0
    mov [FD_C], eax
    CALL5 SYS_CONNECT, [FD_C], UNADDR, 16, 0, 0
    STORE 19                            ; 0
    CALL5 SYS_ACCEPT, [FD_L], 0, 0, 0, 0
    STORE 20                            ; a descriptor
    mov ebx, eax
    mov eax, SYS_CLOSE
    int 0x80

    ; 21: close everything
    CALL5 SYS_CLOSE, [FD_C], 0, 0, 0, 0
    CALL5 SYS_CLOSE, [FD_L], 0, 0, 0, 0
    CALL5 SYS_CLOSE, [EPFD], 0, 0, 0, 0
    CALL5 SYS_CLOSE, [SV + 0], 0, 0, 0, 0
    CALL5 SYS_CLOSE, [SV + 4], 0, 0, 0, 0
    STORE 21                            ; 0

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

hello: db "hello"
path:  db "/tmp/netcalls", 0
local_program_end:
