; =============================================================================
; tcp.s - A ring 3 program that is a TCP server AND its client, over lo:
; socket, bind, listen, connect, accept, sendto/recvfrom, sendmsg/recvmsg
; (several buffers), shutdown (the reader sees end of file), ioctl FIONBIO
; on the listener, then accept4 (nothing waiting: -11 EAGAIN), getpeername.
; (accept4's SOCK_NONBLOCK is for the NEW socket, as in Linux.)
;
; Run by cmd_netcalls.c (group "tcp") through usertask, as user 1 (guest).
; =============================================================================

SYS_EXIT        equ 1
SYS_CLOSE       equ 17
SYS_SOCKET      equ 133
SYS_BIND        equ 134
SYS_CONNECT     equ 135
SYS_LISTEN      equ 136
SYS_ACCEPT      equ 137
SYS_ACCEPT4     equ 138
SYS_GETPEERNAME equ 140
SYS_SENDTO      equ 142
SYS_RECVFROM    equ 143
SYS_SENDMSG     equ 144
SYS_RECVMSG     equ 145
SYS_SHUTDOWN    equ 146
SYS_SETSOCKOPT  equ 147
SYS_IOCTL       equ 165
FIONBIO         equ 0x5421

AF_INET       equ 2
SOCK_STREAM   equ 1
SOCK_NONBLOCK equ 0x800
SOL_TCP       equ 6
TCP_NODELAY   equ 1
SHUT_WR       equ 1
PORT_NET      equ 0x7A1E            ; port 7802 (0x1E7A) in network byte order

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516

ADDR    equ SCRATCH + 0             ; sockaddr_in (16)
PEER    equ SCRATCH + 16            ; sockaddr_in (16)
LEN     equ SCRATCH + 32
VALUE   equ SCRATCH + 36
FD_L    equ SCRATCH + 40            ; the listener
FD_C    equ SCRATCH + 44            ; the client
FD_S    equ SCRATCH + 48            ; the server's end (accept)
IOV     equ SCRATCH + 64            ; 2 iovecs (16)
MSG     equ SCRATCH + 80            ; msghdr (28)
BUF     equ SCRATCH + 128           ; 128 bytes

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

; msghdr in MSG with the two iovecs in IOV: (base0, len0, base1, len1)
%macro SETMSG 4
    mov dword [IOV + 0], %1
    mov dword [IOV + 4], %2
    mov dword [IOV + 8], %3
    mov dword [IOV + 12], %4
    mov dword [MSG + 0], 0              ; no address
    mov dword [MSG + 4], 0
    mov dword [MSG + 8], IOV
    mov dword [MSG + 12], 2
    mov dword [MSG + 16], 0             ; no ancillary data
    mov dword [MSG + 20], 0
    mov dword [MSG + 24], 0
%endmacro

global tcp_program, tcp_program_end

section .text
tcp_program:
    ; 1-3: the server: socket, bind 127.0.0.1:7802, listen
    CALL5 SYS_SOCKET, AF_INET, SOCK_STREAM, 0, 0, 0
    mov [FD_L], eax
    STORE 1
    mov word [ADDR + 0], AF_INET
    mov word [ADDR + 2], PORT_NET
    mov dword [ADDR + 4], 0x0100007F
    CALL5 SYS_BIND, [FD_L], ADDR, 16, 0, 0
    STORE 2                             ; 0
    CALL5 SYS_LISTEN, [FD_L], 4, 0, 0, 0
    STORE 3                             ; 0

    ; 4-6: the client connects; the server accepts
    CALL5 SYS_SOCKET, AF_INET, SOCK_STREAM, 0, 0, 0
    mov [FD_C], eax
    STORE 4
    CALL5 SYS_CONNECT, [FD_C], ADDR, 16, 0, 0
    STORE 5                             ; 0: the handshake is done
    mov dword [LEN], 16
    CALL5 SYS_ACCEPT, [FD_L], PEER, LEN, 0, 0
    mov [FD_S], eax
    STORE 6                             ; a new descriptor

    ; 7-8: who connected? 127.0.0.1
    mov eax, [PEER + 4]
    STORE 7                             ; 0x0100007F
    mov dword [LEN], 16
    CALL5 SYS_GETPEERNAME, [FD_C], PEER, LEN, 0, 0
    STORE 8                             ; 0

    ; 9-10: client -> server, "stream"
    mov dword [BUF], 'stre'
    mov word [BUF + 4], 'am'
    mov eax, SYS_SENDTO
    mov ebx, [FD_C]
    mov ecx, BUF
    mov edx, 6
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    int 0x80
    STORE 9                             ; 6
    mov eax, SYS_RECVFROM
    mov ebx, [FD_S]
    mov ecx, BUF + 64
    mov edx, 64
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    int 0x80
    STORE 10                            ; 6

    ; 11-13: sendmsg from two buffers (3 + 4 bytes), recvmsg into two
    SETMSG BUF, 3, BUF + 3, 4
    CALL5 SYS_SENDMSG, [FD_C], MSG, 0, 0, 0
    STORE 11                            ; 7
    SETMSG BUF + 64, 4, BUF + 68, 10
    CALL5 SYS_RECVMSG, [FD_S], MSG, 0, 0, 0
    STORE 12                            ; 7
    mov eax, [BUF + 64]
    cmp eax, [BUF]
    sete al
    movzx eax, al
    STORE 13                            ; 1: the same bytes arrived

    ; 14: an option on a stream
    mov dword [VALUE], 1
    CALL5 SYS_SETSOCKOPT, [FD_C], SOL_TCP, TCP_NODELAY, VALUE, 4
    STORE 14                            ; 0

    ; 15-16: the client says "no more" (FIN); the server reads end of file
    CALL5 SYS_SHUTDOWN, [FD_C], SHUT_WR, 0, 0, 0
    STORE 15                            ; 0
    mov eax, SYS_RECVFROM
    mov ebx, [FD_S]
    mov ecx, BUF + 64
    mov edx, 64
    xor esi, esi
    xor edi, edi
    xor ebp, ebp
    int 0x80
    STORE 16                            ; 0 = end of file

    ; 17: nothing else is waiting: on a non-blocking listener, accept4 says so
    mov dword [VALUE], 1
    CALL5 SYS_IOCTL, [FD_L], FIONBIO, VALUE, 0, 0
    CALL5 SYS_ACCEPT4, [FD_L], 0, 0, SOCK_NONBLOCK, 0
    STORE 17                            ; -11 EAGAIN

    ; 18-20: close all three
    CALL5 SYS_CLOSE, [FD_S], 0, 0, 0, 0
    STORE 18
    CALL5 SYS_CLOSE, [FD_C], 0, 0, 0, 0
    STORE 19
    CALL5 SYS_CLOSE, [FD_L], 0, 0, 0, 0
    STORE 20

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
tcp_program_end:
