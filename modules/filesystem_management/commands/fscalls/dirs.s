; =============================================================================
; dirs.s - A ring 3 program that works with directories: mkdirat, a
; directory descriptor, getdents64, fstatat / renameat / renameat2 /
; unlinkat relative to it, fchdir, rmdir, and the refusals around them.
;
; Run by cmd_fscalls.c (group "dirs") through usertask, as user 1 (guest).
; =============================================================================

SYS_EXIT       equ 1
SYS_UNLINK     equ 10
SYS_CHDIR      equ 11
SYS_GETCWD     equ 12
SYS_OPEN       equ 14
SYS_CLOSE      equ 17
SYS_FSTATAT    equ 100
SYS_RMDIR      equ 119
SYS_MKDIRAT    equ 120
SYS_UNLINKAT   equ 121
SYS_RENAMEAT   equ 122
SYS_RENAMEAT2  equ 123
SYS_FCHDIR     equ 124
SYS_GETDENTS64 equ 126

AT_FDCWD       equ -100
AT_REMOVEDIR   equ 0x200
NOREPLACE      equ 1
EXCHANGE       equ 2
O_READ         equ 0x01
O_WRITE        equ 0x02
O_RWC          equ 0x07
O_DIRECTORY    equ 0x10000

MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
USER_CODE   equ 0x40000000
DONE        equ 0x516
ST          equ SCRATCH + 0         ; vfs_stat_t
BUF         equ SCRATCH + 64        ; 256 bytes of dirents / cwd
DFD         equ SCRATCH + 340

%define ADDR(label) (USER_CODE + (label - dirs_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

%macro CALL4 6
    mov eax, %1
    mov ebx, %2
    mov ecx, %3
    mov edx, %4
    mov esi, %5
    int 0x80
    STORE %6
%endmacro

global dirs_program, dirs_program_end

section .text

dirs_program:
    CALL4 SYS_MKDIRAT, AT_FDCWD, ADDR(d_path), 0700o, 0, 1
    CALL4 SYS_FSTATAT, AT_FDCWD, ADDR(d_path), ST, 0, 0
    mov eax, [ST + 8]
    and eax, 0777o
    STORE 2                                         ; 0700
    CALL4 SYS_OPEN, ADDR(d_path), O_READ | O_DIRECTORY, 0, 0, 3
    mov [DFD], eax
    CALL4 SYS_OPEN, ADDR(f1_path), O_RWC, 0, 0, 4
    mov ebx, eax                                    ; close it (EAX is about to change)
    mov eax, SYS_CLOSE
    int 0x80
    CALL4 SYS_MKDIRAT, [DFD], ADDR(sub), 0755o, 0, 5     ; relative to the dirfd

    ; ---- list it ----
    CALL4 SYS_GETDENTS64, [DFD], BUF, 256, 0, 6
    mov ecx, eax                                    ; walk the records
    mov esi, BUF
    xor edx, edx
.count:
    test ecx, ecx
    jle .counted
    movzx ebx, word [esi + 16]                      ; d_reclen
    add esi, ebx
    sub ecx, ebx
    inc edx
    jmp .count
.counted:
    mov eax, edx
    STORE 7                                         ; . .. f1 sub
    CALL4 SYS_GETDENTS64, [DFD], BUF, 256, 0, 8     ; the end: 0

    ; ---- relative to the dirfd ----
    CALL4 SYS_FSTATAT, [DFD], ADDR(f1), ST, 0, 9
    CALL4 SYS_RENAMEAT, [DFD], ADDR(f1), [DFD], ADDR(f2), 10
    mov eax, SYS_RENAMEAT2
    mov ebx, [DFD]
    mov ecx, ADDR(f2)
    mov edx, [DFD]
    mov esi, ADDR(sub)
    mov edi, NOREPLACE
    int 0x80
    STORE 11                                        ; sub exists: EEXIST
    mov eax, SYS_RENAMEAT2
    mov ebx, [DFD]
    mov ecx, ADDR(f2)
    mov edx, [DFD]
    mov esi, ADDR(sub)
    mov edi, EXCHANGE
    int 0x80
    STORE 12                                        ; swapped
    CALL4 SYS_FSTATAT, [DFD], ADDR(sub), ST, 0, 0
    mov eax, [ST + 8]
    shr eax, 12
    STORE 13                                        ; "sub" is the file now: 8
    CALL4 SYS_UNLINKAT, [DFD], ADDR(f2), 0, 0, 14            ; f2 is a directory now
    CALL4 SYS_UNLINKAT, [DFD], ADDR(f2), AT_REMOVEDIR, 0, 15
    CALL4 SYS_UNLINK, ADDR(d_path), 0, 0, 0, 16     ; a directory: EISDIR
    CALL4 SYS_RMDIR, ADDR(d_path), 0, 0, 0, 17      ; not empty

    ; ---- fchdir ----
    CALL4 SYS_FCHDIR, [DFD], 0, 0, 0, 18
    CALL4 SYS_GETCWD, BUF, 64, 0, 0, 19
    mov eax, [BUF]
    STORE 20                                        ; "/hom"
    CALL4 SYS_UNLINKAT, AT_FDCWD, ADDR(sub), 0, 0, 21        ; relative to the new cwd
    CALL4 SYS_CHDIR, ADDR(home), 0, 0, 0, 22
    CALL4 SYS_CLOSE, [DFD], 0, 0, 0, 23
    CALL4 SYS_RMDIR, ADDR(d_path), 0, 0, 0, 24

    ; ---- refusals ----
    CALL4 SYS_OPEN, ADDR(hello), O_READ | O_DIRECTORY, 0, 0, 25  ; a file
    CALL4 SYS_OPEN, ADDR(home), O_WRITE, 0, 0, 26                ; a directory, to write
    CALL4 SYS_OPEN, ADDR(root_home), O_READ | O_DIRECTORY, 0, 0, 27  ; root's: private
    CALL4 SYS_OPEN, ADDR(hello), O_READ, 0, 0, 0
    mov [DFD], eax
    CALL4 SYS_GETDENTS64, [DFD], BUF, 256, 0, 28    ; not a directory
    CALL4 SYS_CLOSE, [DFD], 0, 0, 0, 0

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

d_path:    db "/home/guest/d", 0
f1_path:   db "/home/guest/d/f1", 0
home:      db "/home/guest", 0
root_home: db "/home/root", 0
hello:     db "/hello", 0
sub:       db "sub", 0
f1:        db "f1", 0
f2:        db "f2", 0
dirs_program_end:
