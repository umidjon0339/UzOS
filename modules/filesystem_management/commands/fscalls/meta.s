; =============================================================================
; meta.s - A ring 3 program that reads and changes file metadata: stat,
; lstat, fstat, fstatat, statx, access, faccessat, chmod, fchmod, chown,
; umask, truncate, ftruncate, fallocate, utime, utimes, utimensat.
;
; Run by cmd_fscalls.c (group "meta") through usertask, as user 1 (guest),
; in its home /home/guest. Each answer goes into one mailbox slot.
; Numbers match modules/filesystem_management/syscalls/<name>/sys_<name>.h.
; =============================================================================

SYS_EXIT       equ 1
SYS_UNLINK     equ 10
SYS_OPEN       equ 14
SYS_WRITE      equ 16
SYS_CLOSE      equ 17
SYS_STAT       equ 97
SYS_LSTAT      equ 98
SYS_FSTAT      equ 99
SYS_FSTATAT    equ 100
SYS_STATX      equ 101
SYS_ACCESS     equ 102
SYS_FACCESSAT  equ 103
SYS_CHMOD      equ 105
SYS_FCHMOD     equ 106
SYS_CHOWN      equ 108
SYS_UMASK      equ 112
SYS_TRUNCATE   equ 113
SYS_FTRUNCATE  equ 114
SYS_FALLOCATE  equ 115
SYS_UTIME      equ 116
SYS_UTIMES     equ 117
SYS_UTIMENSAT  equ 118

AT_FDCWD    equ -100
O_RWC       equ 0x01 | 0x02 | 0x04

USER_CODE   equ 0x40000000
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000
DONE        equ 0x516

ST          equ SCRATCH + 0         ; vfs_stat_t (52)
SX          equ SCRATCH + 64        ; statx_t (76)
TSTAMPS       equ SCRATCH + 160       ; two {sec, nsec/usec}
FD          equ SCRATCH + 176
FD2         equ SCRATCH + 180

ST_MODE     equ ST + 8
ST_NLINK    equ ST + 12
ST_UID      equ ST + 16
ST_SIZE     equ ST + 28
ST_ATIME    equ ST + 40
ST_MTIME    equ ST + 44
ST_INO      equ ST + 4

%define ADDR(label) (USER_CODE + (label - meta_program))

%macro STORE 1
    mov [MAILBOX + %1 * 4], eax
%endmacro

%macro CALL3 5
    mov eax, %1
    mov ebx, %2
    mov ecx, %3
    mov edx, %4
    int 0x80
    STORE %5
%endmacro

; stat(path) into ST (result not stored)
%macro STAT 1
    mov eax, SYS_STAT
    mov ebx, %1
    mov ecx, ST
    int 0x80
%endmacro

; the permission bits of ST -> slot
%macro MODEBITS 1
    mov eax, [ST_MODE]
    and eax, 0777o
    STORE %1
%endmacro

global meta_program, meta_program_end

section .text

meta_program:
    CALL3 SYS_OPEN, ADDR(file), O_RWC, 0, 1
    mov [FD], eax
    CALL3 SYS_WRITE, [FD], ADDR(hello), 5, 2

    ; ---- the stat family ----
    CALL3 SYS_STAT, ADDR(file), ST, 0, 3
    mov eax, [ST_SIZE]
    STORE 4
    mov eax, [ST_MODE]
    STORE 5                                 ; S_IFREG | 0644
    mov eax, [ST_UID]
    STORE 6
    mov eax, [ST_NLINK]
    STORE 7
    mov eax, [ST_INO]
    STORE 8
    CALL3 SYS_FSTAT, [FD], ST, 0, 9
    mov eax, [ST_SIZE]
    STORE 10
    mov eax, SYS_FSTATAT
    mov ebx, AT_FDCWD
    mov ecx, ADDR(file)
    mov edx, ST
    xor esi, esi
    int 0x80
    STORE 11
    mov eax, SYS_STATX
    mov ebx, AT_FDCWD
    mov ecx, ADDR(file)
    xor edx, edx
    mov esi, 0x7FF
    mov edi, SX
    int 0x80
    STORE 12
    mov eax, [SX]
    STORE 13                                ; the mask it filled
    CALL3 SYS_LSTAT, ADDR(file), ST, 0, 14

    ; ---- access ----
    CALL3 SYS_ACCESS, ADDR(file), 6, 0, 15  ; R_OK | W_OK: mine
    CALL3 SYS_ACCESS, ADDR(hello_path), 2, 0, 16    ; root's /hello: no write
    CALL3 SYS_ACCESS, ADDR(hello_path), 4, 0, 17    ; ...but read
    CALL3 SYS_FACCESSAT, AT_FDCWD, ADDR(file), 6, 18

    ; ---- chmod, chown ----
    CALL3 SYS_CHMOD, ADDR(file), 0600o, 0, 19
    STAT ADDR(file)
    MODEBITS 20
    CALL3 SYS_CHMOD, ADDR(hello_path), 0666o, 0, 21 ; not mine: refused
    CALL3 SYS_CHOWN, ADDR(file), 0, -1, 22          ; give it to root: no CAP_CHOWN
    CALL3 SYS_FCHMOD, [FD], 0640o, 0, 23
    STAT ADDR(file)
    MODEBITS 24

    ; ---- umask ----
    CALL3 SYS_UMASK, 077o, 0, 0, 25                 ; the old one: 022
    CALL3 SYS_OPEN, ADDR(file2), O_RWC, 0, 26
    mov ebx, eax                                    ; close it (EAX is about to change)
    mov eax, SYS_CLOSE
    int 0x80
    STAT ADDR(file2)
    MODEBITS 27                                     ; 0600
    CALL3 SYS_UMASK, 0, 0, 0, 28                    ; the old one: 077
    CALL3 SYS_OPEN, ADDR(file3), O_RWC, 0, 0
    mov ebx, eax                                    ; close it (EAX is about to change)
    mov eax, SYS_CLOSE
    int 0x80
    STAT ADDR(file3)
    MODEBITS 29                                     ; 0644: secure_umask
    CALL3 SYS_UMASK, 022o, 0, 0, 0

    ; ---- size ----
    CALL3 SYS_FTRUNCATE, [FD], 2, 0, 30
    CALL3 SYS_FSTAT, [FD], ST, 0, 0
    mov eax, [ST_SIZE]
    STORE 31
    CALL3 SYS_TRUNCATE, ADDR(file), 10, 0, 32
    STAT ADDR(file)
    mov eax, [ST_SIZE]
    STORE 33
    CALL3 SYS_TRUNCATE, ADDR(file), 0x7FFFFFFF, 0, 34   ; size_cap
    mov eax, SYS_FALLOCATE
    mov ebx, [FD]
    xor ecx, ecx
    xor edx, edx
    mov esi, 100
    int 0x80
    STORE 35
    STAT ADDR(file)
    mov eax, [ST_SIZE]
    STORE 36

    ; ---- times ----
    mov dword [TSTAMPS + 0], 1000
    mov dword [TSTAMPS + 4], 0
    mov dword [TSTAMPS + 8], 2000
    mov dword [TSTAMPS + 12], 0
    mov eax, SYS_UTIMENSAT
    mov ebx, AT_FDCWD
    mov ecx, ADDR(file)
    mov edx, TSTAMPS
    xor esi, esi
    int 0x80
    STORE 37
    STAT ADDR(file)
    mov eax, [ST_ATIME]
    STORE 38
    mov eax, [ST_MTIME]
    STORE 39
    mov eax, SYS_UTIMENSAT
    mov ebx, AT_FDCWD
    mov ecx, ADDR(hello_path)
    xor edx, edx                            ; "now", on root's file: not a writer
    xor esi, esi
    int 0x80
    STORE 40
    CALL3 SYS_UTIME, ADDR(file), 0, 0, 41   ; now
    mov dword [TSTAMPS + 0], 5
    mov dword [TSTAMPS + 8], 6
    CALL3 SYS_UTIMES, ADDR(file), TSTAMPS, 0, 42
    STAT ADDR(file)
    mov eax, [ST_MTIME]
    STORE 43

    CALL3 SYS_CLOSE, [FD], 0, 0, 0
    CALL3 SYS_UNLINK, ADDR(file), 0, 0, 0
    CALL3 SYS_UNLINK, ADDR(file2), 0, 0, 0
    CALL3 SYS_UNLINK, ADDR(file3), 0, 0, 0
    mov dword [MAILBOX], DONE
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80

file:       db "/home/guest/meta1", 0
file2:      db "/home/guest/meta2", 0
file3:      db "/home/guest/meta3", 0
hello_path: db "/hello", 0
hello:      db "hello"
meta_program_end:
