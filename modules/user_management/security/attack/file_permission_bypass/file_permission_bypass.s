; =============================================================================
; file_permission_bypass.s - A ring 3 program (guest) goes after root's files.
;
; Used by atk_file_permission_bypass.c through usertask (process_management).
; Each answer (a descriptor or 0 = success, -1 = refused) goes into the
; mailbox. The paths live in this code page, so they are the program's
; own memory, as the syscall handler requires.
; =============================================================================

SYS_EXIT   equ 1
SYS_MKDIR  equ 9
SYS_UNLINK equ 10
SYS_OPEN   equ 14
O_READ     equ 1
O_WRITE    equ 2
MAILBOX    equ 0x40000F00       ; = USERTASK_MAILBOX in usertask.h
DONE       equ 0x600D           ; mailbox[0]: "finished"

global perm_program, perm_program_end

section .text

perm_program:
    ; This code runs at a different address than it was assembled for:
    ; 'call' pushes where we are, so the strings can be found from there.
    call .here
.here:
    pop esi

    mov eax, SYS_OPEN           ; 1. read root's private file
    lea ebx, [esi + secret - .here]
    mov ecx, O_READ
    int 0x80
    mov [MAILBOX + 4], eax

    mov eax, SYS_OPEN           ; 2. overwrite a file root owns
    lea ebx, [esi + rootfile - .here]
    mov ecx, O_WRITE
    int 0x80
    mov [MAILBOX + 8], eax

    mov eax, SYS_UNLINK         ; 3. delete it
    lea ebx, [esi + rootfile - .here]
    int 0x80
    mov [MAILBOX + 12], eax

    mov eax, SYS_MKDIR          ; 4. create something in root's "/"
    lea ebx, [esi + evil - .here]
    int 0x80
    mov [MAILBOX + 16], eax

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    int 0x80

secret:   db "/home/root/secret", 0
rootfile: db "/seclab_root_file", 0
evil:     db "/seclab_evil", 0
perm_program_end:
