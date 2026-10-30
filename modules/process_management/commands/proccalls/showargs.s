; =============================================================================
; showargs.s - A complete ELF program, built byte by byte: no compiler,
; no linker. cmd_proccalls.c writes these bytes to /home/guest/showargs,
; and exec.s runs it with execve().
;
;   offset 0    Elf32_Ehdr    "this is a 32-bit i386 executable, start at e_entry"
;   offset 52   Elf32_Phdr    "load the whole file at 0x48000000"
;   offset 84   the code      reads its argc / argv / envp from the stack,
;                             as execve() built it, and reports them
;
; It runs in ring 3 and talks to the world only through int 0x80. It
; writes what it saw into the usertask mailbox (the same address space):
;   [40] argc   [41] argv[1] (4 bytes)   [42] envc   [43] envp[0] (4 bytes)
;   [44] read(3): the parent's O_CLOEXEC file must be closed   [45] getpid
; then exit(42).
; =============================================================================

LOAD     equ 0x48000000             ; where the file is loaded (user space)
MAILBOX  equ 0x40000F00

%define AT(label) (LOAD + (label - showargs_elf))

global showargs_elf, showargs_elf_end

section .rodata

showargs_elf:
; ---- Elf32_Ehdr ----
    db 0x7F, "ELF", 1, 1, 1, 0       ; magic, 32-bit, little-endian, version 1
    times 8 db 0
    dw 2                             ; e_type: executable
    dw 3                             ; e_machine: i386
    dd 1                             ; e_version
    dd AT(start)                     ; e_entry
    dd phdr - showargs_elf           ; e_phoff
    dd 0                             ; e_shoff: no sections needed to run
    dd 0                             ; e_flags
    dw 52                            ; e_ehsize
    dw 32                            ; e_phentsize
    dw 1                             ; e_phnum
    dw 0, 0, 0                       ; e_shentsize, e_shnum, e_shstrndx
; ---- Elf32_Phdr ----
phdr:
    dd 1                             ; p_type: PT_LOAD
    dd 0                             ; p_offset: from the start of the file
    dd LOAD                          ; p_vaddr
    dd LOAD                          ; p_paddr
    dd showargs_elf_end - showargs_elf   ; p_filesz
    dd showargs_elf_end - showargs_elf   ; p_memsz
    dd 5                             ; p_flags: read + execute
    dd 0x1000                        ; p_align
; ---- the code ----
start:
    mov eax, [esp]                   ; argc
    mov [MAILBOX + 40 * 4], eax
    mov ebx, [esp + 8]               ; argv[1]
    test ebx, ebx
    jz .no_arg
    mov eax, [ebx]
    mov [MAILBOX + 41 * 4], eax
.no_arg:
    mov ecx, [esp]
    lea esi, [esp + 8 + ecx * 4]     ; envp: past argc, argv[], NULL
    xor edx, edx
.count:
    cmp dword [esi + edx * 4], 0
    je .counted
    inc edx
    jmp .count
.counted:
    mov [MAILBOX + 42 * 4], edx
    mov eax, [esi]
    test eax, eax
    jz .no_env
    mov eax, [eax]
    mov [MAILBOX + 43 * 4], eax
.no_env:
    sub esp, 16
    mov eax, 15                      ; read(3, buffer, 1)
    mov ebx, 3
    mov ecx, esp
    mov edx, 1
    int 0x80
    mov [MAILBOX + 44 * 4], eax
    mov eax, 2                       ; getpid()
    int 0x80
    mov [MAILBOX + 45 * 4], eax
    mov eax, 1                       ; exit(42)
    mov ebx, 42
    int 0x80
showargs_elf_end:
