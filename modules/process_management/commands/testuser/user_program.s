; =============================================================================
; user_program.s - Tiny programs `testuser` runs in ring 3.
;
; They are copied to a user page, so they may only use relative jumps
; (no absolute addresses of their own). In ring 3 they cannot touch
; hardware or kernel memory: the only way to get anything done is to ask
; the kernel with int 0x80. Numbers match syscalls/<name>/sys_<name>.h.
; =============================================================================

SYS_EXIT   equ 1
SYS_GETPID equ 2
SYS_MMAP   equ 6
SYS_MUNMAP equ 7
SYS_UPTIME equ 8
SYS_WRITE  equ 16
STDOUT     equ 1
PAGE_SIZE  equ 4096

global user_program
global user_program_end
global user_null_program
global user_null_program_end
global user_badptr_program
global user_badptr_program_end

section .text

; --- `testuser`: use a few syscalls, then exit cleanly -----------------------
user_program:
    ; Where is our message? This code runs at a different address than it
    ; was assembled for, so ask the CPU: 'call' pushes the address of the
    ; next instruction, and we pop it.
    call .here
.here:
    pop esi                     ; esi = address of .here

    mov eax, SYS_WRITE
    mov ebx, STDOUT
    lea ecx, [esi + message - .here]
    mov edx, message_end - message
    int 0x80                    ; print the message

    mov eax, SYS_GETPID
    int 0x80                    ; eax = our PID

    mov eax, SYS_UPTIME
    int 0x80                    ; eax = milliseconds since boot

    mov eax, SYS_MMAP
    mov ebx, PAGE_SIZE
    int 0x80                    ; eax = address of a fresh page, or 0
    test eax, eax
    jz .exit

    mov dword [eax], 0x12345678 ; it is ours: ring 3 may write to it

    mov ebx, eax                ; munmap(address, PAGE_SIZE)
    mov ecx, PAGE_SIZE
    mov eax, SYS_MUNMAP
    int 0x80

.exit:
    mov eax, SYS_EXIT
    int 0x80                    ; the kernel ends us: never comes back
.hang:
    jmp .hang

message:
    db "  hello from ring 3!", 10
message_end:
user_program_end:

; --- `testuser null`: a NULL pointer bug -------------------------------------
user_null_program:
    xor eax, eax
    mov dword [eax], 1          ; write to address 0: page fault
.hang:
    jmp .hang                   ; (not reached: the kernel kills us)
user_null_program_end:

; --- `testuser badptr`: hand the kernel a pointer to unmapped memory ---------
user_badptr_program:
    mov eax, SYS_WRITE
    mov ebx, STDOUT
    mov ecx, 0x70000000         ; user space, but nothing is mapped there
    mov edx, 16
    int 0x80                    ; refused: eax = -1, the kernel is fine

    mov eax, SYS_EXIT
    int 0x80
.hang:
    jmp .hang
user_badptr_program_end:
