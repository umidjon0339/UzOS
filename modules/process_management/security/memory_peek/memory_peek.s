; =============================================================================
; memory_peek.s - A ring 3 program that reads memory it must not.
;
; Used by atk_memory_peek.c through usertask, as user 1.
;   1. process_vm_readv(1, ...)              idle: a root system process
;   2. process_vm_readv(me, remote = kernel) 0x00100000: the kernel's code
;   3. process_vm_readv(me, remote = my own) the honest case: must work
; Mailbox: [1..3] = the answers (bytes read, or -1), [0] = 1 when done.
; =============================================================================

SYS_EXIT             equ 1
SYS_GETPID           equ 2
SYS_PROCESS_VM_READV equ 94

MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
LOCAL       equ SCRATCH + 0     ; struct iovec (8)
REMOTE      equ SCRATCH + 8     ; struct iovec (8)
BUF         equ SCRATCH + 16    ; 16 bytes
MINE        equ SCRATCH + 32    ; 16 bytes of our own to read back

%macro READV 3
    mov dword [REMOTE + 0], %2
    mov dword [REMOTE + 4], 16
    mov eax, SYS_PROCESS_VM_READV
    mov ebx, %1
    mov ecx, LOCAL
    mov edx, 1
    mov esi, REMOTE
    mov edi, 1
    xor ebp, ebp
    int 0x80
    mov [MAILBOX + %3 * 4], eax
%endmacro

global peek_program, peek_program_end

section .text

peek_program:
    mov dword [LOCAL + 0], BUF
    mov dword [LOCAL + 4], 16
    mov eax, SYS_GETPID
    int 0x80
    mov edi, eax                    ; our pid, kept in a register
    push edi
    READV 1, MINE, 1                ; idle's memory
    pop edi
    push edi
    READV edi, 0x00100000, 2        ; the kernel, as "remote" memory
    pop edi
    READV edi, MINE, 3              ; our own: fine
    mov dword [MAILBOX], 1
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
peek_program_end:
