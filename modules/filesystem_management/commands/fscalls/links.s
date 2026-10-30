; =============================================================================
; links.s - A ring 3 program that makes hard and symbolic links and walks
; through them: link, linkat, symlink, symlinkat, readlink, readlinkat,
; lstat vs stat, ELOOP, a file outliving its first name.
;
; Run by cmd_fscalls.c (group "links") through usertask, as user 1 (guest).
; =============================================================================

SYS_EXIT       equ 1
SYS_UNLINK     equ 10
SYS_OPEN       equ 14
SYS_READ       equ 15
SYS_WRITE      equ 16
SYS_CLOSE      equ 17
SYS_STAT       equ 97
SYS_LSTAT      equ 98
SYS_LINK       equ 127
SYS_LINKAT     equ 128
SYS_SYMLINK    equ 129
SYS_SYMLINKAT  equ 130
SYS_READLINK   equ 131
SYS_READLINKAT equ 132

AT_FDCWD          equ -100
AT_SYMLINK_FOLLOW equ 0x400
O_READ            equ 0x01
O_RWC             equ 0x07

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516
ST          equ SCRATCH + 0
INO         equ SCRATCH + 60
BUF         equ SCRATCH + 64
FD          equ SCRATCH + 200

%define ADDR(label) (USER_CODE + (label - links_program))

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

global links_program, links_program_end

section .text

links_program:
    CALL4 SYS_OPEN, ADDR(l1), O_RWC, 0, 0, 1
    mov ebx, eax
    push ebx
    mov eax, SYS_WRITE
    mov ecx, ADDR(abc)
    mov edx, 3
    int 0x80
    STORE 2
    pop ebx
    mov eax, SYS_CLOSE
    int 0x80

    ; ---- a hard link: two names, one inode ----
    CALL4 SYS_LINK, ADDR(l1), ADDR(l2), 0, 0, 3
    CALL4 SYS_STAT, ADDR(l2), ST, 0, 0, 0
    mov eax, [ST + 12]
    STORE 4                                 ; nlink 2
    mov eax, [ST + 4]
    mov [INO], eax
    CALL4 SYS_STAT, ADDR(l1), ST, 0, 0, 0
    mov eax, [ST + 4]
    cmp eax, [INO]
    sete al
    movzx eax, al
    STORE 5                                 ; same inode

    ; ---- a symbolic link ----
    CALL4 SYS_SYMLINK, ADDR(rel_l1), ADDR(s1), 0, 0, 6
    CALL4 SYS_READLINK, ADDR(s1), BUF, 64, 0, 7
    movzx eax, word [BUF]
    STORE 8                                 ; "l1"
    CALL4 SYS_LSTAT, ADDR(s1), ST, 0, 0, 0
    mov eax, [ST + 8]
    shr eax, 12
    STORE 9                                 ; the link itself: 10
    CALL4 SYS_STAT, ADDR(s1), ST, 0, 0, 0
    mov eax, [ST + 8]
    shr eax, 12
    STORE 10                                ; where it points: a file, 8
    mov eax, [ST + 28]
    STORE 11                                ; its size: 3
    CALL4 SYS_READLINK, ADDR(l1), BUF, 64, 0, 12    ; not a link

    ; ---- refusals ----
    CALL4 SYS_LINK, ADDR(hello), ADDR(h), 0, 0, 13  ; root's file: protected_hardlinks
    CALL4 SYS_LINK, ADDR(home), ADDR(dirlink), 0, 0, 14  ; a directory
    CALL4 SYS_SYMLINK, ADDR(rel_loopb), ADDR(loopa), 0, 0, 15
    CALL4 SYS_SYMLINK, ADDR(rel_loopa), ADDR(loopb), 0, 0, 16
    CALL4 SYS_STAT, ADDR(loopa), ST, 0, 0, 17       ; ELOOP

    ; ---- a file outlives its first name ----
    CALL4 SYS_UNLINK, ADDR(l1), 0, 0, 0, 18
    CALL4 SYS_STAT, ADDR(l2), ST, 0, 0, 0
    mov eax, [ST + 12]
    STORE 19                                ; nlink 1
    CALL4 SYS_OPEN, ADDR(l2), O_READ, 0, 0, 20
    mov [FD], eax
    CALL4 SYS_READ, [FD], BUF, 16, 0, 21    ; "abc": 3
    CALL4 SYS_CLOSE, [FD], 0, 0, 0, 0
    CALL4 SYS_STAT, ADDR(s1), ST, 0, 0, 22  ; s1 -> l1: gone now (dangling)

    ; ---- the *at versions ----
    mov eax, SYS_LINKAT
    mov ebx, AT_FDCWD
    mov ecx, ADDR(l2)
    mov edx, AT_FDCWD
    mov esi, ADDR(l3)
    xor edi, edi
    int 0x80
    STORE 23
    CALL4 SYS_SYMLINKAT, ADDR(rel_l2), AT_FDCWD, ADDR(s2), 0, 24
    CALL4 SYS_READLINKAT, AT_FDCWD, ADDR(s2), BUF, 64, 25
    mov eax, SYS_LINKAT
    mov ebx, AT_FDCWD
    mov ecx, ADDR(s2)                       ; through the link, to l2's file
    mov edx, AT_FDCWD
    mov esi, ADDR(l4)
    mov edi, AT_SYMLINK_FOLLOW
    int 0x80
    STORE 26
    CALL4 SYS_STAT, ADDR(l4), ST, 0, 0, 0
    mov eax, [ST + 12]
    STORE 27                                ; l2, l3, l4: 3 names

    ; tidy up
    CALL4 SYS_UNLINK, ADDR(l2), 0, 0, 0, 0
    CALL4 SYS_UNLINK, ADDR(l3), 0, 0, 0, 0
    CALL4 SYS_UNLINK, ADDR(l4), 0, 0, 0, 0
    CALL4 SYS_UNLINK, ADDR(s1), 0, 0, 0, 0
    CALL4 SYS_UNLINK, ADDR(s2), 0, 0, 0, 0
    CALL4 SYS_UNLINK, ADDR(loopa), 0, 0, 0, 0
    CALL4 SYS_UNLINK, ADDR(loopb), 0, 0, 0, 0

    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

l1:        db "/home/guest/l1", 0
l2:        db "/home/guest/l2", 0
l3:        db "/home/guest/l3", 0
l4:        db "/home/guest/l4", 0
s1:        db "/home/guest/s1", 0
s2:        db "/home/guest/s2", 0
loopa:     db "/home/guest/loopa", 0
loopb:     db "/home/guest/loopb", 0
h:         db "/home/guest/h", 0
dirlink:   db "/home/guest/dl", 0
home:      db "/home/guest", 0
hello:     db "/hello", 0
rel_l1:    db "l1", 0
rel_l2:    db "l2", 0
rel_loopa: db "loopa", 0
rel_loopb: db "loopb", 0
abc:       db "abc"
links_program_end:
