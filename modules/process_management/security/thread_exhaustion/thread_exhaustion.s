; =============================================================================
; thread_exhaustion.s - A ring 3 program that starts threads until the
; kernel says no.
;
; Used by atk_thread_exhaustion.c through usertask (process_management).
; It calls clone(CLONE_VM | CLONE_SIGHAND | CLONE_THREAD) 10 times; each
; thread waits in pause() on its own small stack. Then exit_group() ends
; them all. Mailbox: [1] = threads started, [0] = 1 when done.
; =============================================================================

SYS_CLONE      equ 43
SYS_EXIT_GROUP equ 51
SYS_PAUSE      equ 78
THREAD_FLAGS   equ 0x00010900       ; CLONE_VM | CLONE_SIGHAND | CLONE_THREAD
TRIES          equ 10

MAILBOX        equ 0x40000F00
STACKS         equ 0x40001100       ; thread i: stack top at STACKS + i * 0x100

global threads_program, threads_program_end

section .text

threads_program:
    xor esi, esi                    ; i
    xor edi, edi                    ; started
.again:
    mov ecx, esi
    shl ecx, 8
    add ecx, STACKS + 0x100         ; this thread's stack top
    mov eax, SYS_CLONE
    mov ebx, THREAD_FLAGS
    xor edx, edx
    push edi
    xor edi, edi
    int 0x80
    pop edi
    test eax, eax
    jz .thread                      ; we are the new thread
    js .next                        ; refused
    inc edi
.next:
    inc esi
    cmp esi, TRIES
    jb .again
    mov [MAILBOX + 4], edi
    mov dword [MAILBOX], 1
    mov eax, SYS_EXIT_GROUP         ; end every thread of the program
    xor ebx, ebx
    int 0x80
.thread:
    mov eax, SYS_PAUSE
    int 0x80
    jmp .thread
threads_program_end:
