; =============================================================================
; udp.s - A ring 3 program that uses UDP sockets the way a C program would:
; socket, bind, getsockname, sendto, recvfrom, connect, read/write on a
; socket, options, socketcall, uname / gethostname, and the refusals a
; normal user must get (raw socket, port 53, sethostname, short option).
;
; Run by cmd_netcalls.c (group "udp") through usertask, as user 1 (guest).
; Every answer goes to a MAILBOX slot; cmd_netcalls.c checks them.
; Errors come back as -errno (-1 EPERM, -13 EACCES, -22 EINVAL, -107 ENOTCONN).
; =============================================================================

SYS_EXIT        equ 1
SYS_READ        equ 15
SYS_WRITE       equ 16
SYS_CLOSE       equ 17
SYS_SOCKET      equ 133
SYS_BIND        equ 134
SYS_CONNECT     equ 135
SYS_GETSOCKNAME equ 139
SYS_GETPEERNAME equ 140
SYS_SENDTO      equ 142
SYS_RECVFROM    equ 143
SYS_SETSOCKOPT  equ 147
SYS_GETSOCKOPT  equ 148
SYS_SOCKETCALL  equ 149
SYS_UNAME       equ 161
SYS_SETHOSTNAME equ 162
SYS_GETHOSTNAME equ 164

AF_INET     equ 2
SOCK_DGRAM  equ 2
SOCK_RAW    equ 3
SOL_SOCKET  equ 1
SO_TYPE     equ 3
SO_RCVBUF   equ 8
IPPROTO_ICMP equ 1

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000          ; the bottom of the stack page
DONE        equ 0x516

ADDR_A  equ SCRATCH + 0             ; sockaddr_in (16): socket A's address
ADDR_B  equ SCRATCH + 16            ; sockaddr_in (16): where a datagram came from
LEN     equ SCRATCH + 32            ; socklen (4)
VALUE   equ SCRATCH + 36            ; an option value (4)
FD_A    equ SCRATCH + 40
FD_B    equ SCRATCH + 44
ARGS    equ SCRATCH + 48            ; socketcall's argument array (12)
BUF     equ SCRATCH + 64            ; 64 bytes
UTS     equ SCRATCH + 128           ; utsname (6 x 65 = 390)

%define ADDR(label) (USER_CODE + (label - udp_program))

; mailbox[slot] = eax
%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

; eax = syscall(n, b, c, d, s, D)  - up to five arguments
%macro CALL5 6
    mov eax, %1
    mov ebx, %2
    mov ecx, %3
    mov edx, %4
    mov esi, %5
    mov edi, %6
    int 0x80
%endmacro

global udp_program, udp_program_end

section .text
udp_program:
    ; 1-2: two UDP sockets
    CALL5 SYS_SOCKET, AF_INET, SOCK_DGRAM, 0, 0, 0
    mov [FD_A], eax
    STORE 1                             ; a descriptor (>= 3)
    CALL5 SYS_SOCKET, AF_INET, SOCK_DGRAM, 0, 0, 0
    mov [FD_B], eax
    STORE 2

    ; 3-4: A binds 127.0.0.1, port 0 (any); getsockname tells which
    mov word [ADDR_A + 0], AF_INET
    mov word [ADDR_A + 2], 0
    mov dword [ADDR_A + 4], 0x0100007F  ; 127.0.0.1, network byte order
    CALL5 SYS_BIND, [FD_A], ADDR_A, 16, 0, 0
    STORE 3                             ; 0
    mov dword [LEN], 16
    CALL5 SYS_GETSOCKNAME, [FD_A], ADDR_A, LEN, 0, 0
    STORE 4                             ; 0
    movzx eax, word [ADDR_A + 2]
    xchg al, ah                         ; the port, back to host order
    STORE 5                             ; an ephemeral port (>= 32768)

    ; 6-8: B sends "hello" to A; A receives it, and who sent it
    mov dword [BUF], 'hell'
    mov byte [BUF + 4], 'o'
    mov eax, SYS_SENDTO
    mov ebx, [FD_B]
    mov ecx, BUF
    mov edx, 5
    xor esi, esi
    mov edi, ADDR_A
    mov ebp, 16                         ; the 6th argument rides in EBP
    int 0x80
    STORE 6                             ; 5 bytes
    mov dword [BUF + 32], 0
    mov dword [LEN], 16
    mov eax, SYS_RECVFROM
    mov ebx, [FD_A]
    mov ecx, BUF + 32
    mov edx, 32
    xor esi, esi
    mov edi, ADDR_B
    mov ebp, LEN
    int 0x80
    STORE 7                             ; 5 bytes
    mov eax, [BUF + 32]
    cmp eax, 'hell'
    sete al
    movzx eax, al
    STORE 8                             ; 1: the same bytes

    ; 9-12: connect B to A; then plain write() / read() work on sockets
    CALL5 SYS_GETPEERNAME, [FD_A], ADDR_B, LEN, 0, 0
    STORE 9                             ; -107 ENOTCONN: A is not connected
    CALL5 SYS_CONNECT, [FD_B], ADDR_A, 16, 0, 0
    STORE 10                            ; 0
    CALL5 SYS_WRITE, [FD_B], BUF, 2, 0, 0
    STORE 11                            ; 2
    CALL5 SYS_READ, [FD_A], BUF + 32, 32, 0, 0
    STORE 12                            ; 2

    ; 13-16: options
    mov dword [VALUE], 8192
    CALL5 SYS_SETSOCKOPT, [FD_A], SOL_SOCKET, SO_RCVBUF, VALUE, 4
    STORE 13                            ; 0
    mov dword [LEN], 4
    CALL5 SYS_GETSOCKOPT, [FD_A], SOL_SOCKET, SO_TYPE, VALUE, LEN
    STORE 14                            ; 0
    mov eax, [VALUE]
    STORE 15                            ; 2 = SOCK_DGRAM
    CALL5 SYS_SETSOCKOPT, [FD_A], SOL_SOCKET, SO_RCVBUF, VALUE, 1
    STORE 16                            ; -22: 1 byte is no int (optlen_check)

    ; 17-19: what a normal user may not do
    CALL5 SYS_SOCKET, AF_INET, SOCK_RAW, IPPROTO_ICMP, 0, 0
    STORE 17                            ; -1 EPERM (raw_cap)
    CALL5 SYS_SOCKET, AF_INET, SOCK_DGRAM, 0, 0, 0
    push eax
    mov word [ADDR_B + 0], AF_INET
    mov word [ADDR_B + 2], 0x3500       ; port 53
    mov dword [ADDR_B + 4], 0
    CALL5 SYS_BIND, [esp], ADDR_B, 16, 0, 0
    STORE 18                            ; -13 EACCES (port_privilege)
    pop ebx
    mov eax, SYS_CLOSE
    int 0x80
    CALL5 SYS_SETHOSTNAME, ADDR(name), 4, 0, 0, 0
    STORE 19                            ; -1 EPERM

    ; 20-21: the old door: socketcall(SOCKET, {AF_INET, SOCK_DGRAM, 0})
    mov dword [ARGS + 0], AF_INET
    mov dword [ARGS + 4], SOCK_DGRAM
    mov dword [ARGS + 8], 0
    CALL5 SYS_SOCKETCALL, 1, ARGS, 0, 0, 0
    STORE 20                            ; a descriptor
    mov ebx, eax
    mov eax, SYS_CLOSE
    int 0x80
    STORE 21                            ; 0

    ; 22-25: names
    CALL5 SYS_UNAME, UTS, 0, 0, 0, 0
    STORE 22                            ; 0
    mov eax, [UTS]
    STORE 23                            ; "UzOS" as a number
    CALL5 SYS_GETHOSTNAME, BUF, 64, 0, 0, 0
    STORE 24                            ; 0
    CALL5 SYS_GETHOSTNAME, BUF, 2, 0, 0, 0
    STORE 25                            ; -22: no room for the name

    ; 26-27: close both
    CALL5 SYS_CLOSE, [FD_A], 0, 0, 0, 0
    STORE 26
    CALL5 SYS_CLOSE, [FD_B], 0, 0, 0, 0
    STORE 27

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

name: db "evil"
udp_program_end:
