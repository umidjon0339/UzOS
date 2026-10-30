; =============================================================================
; vfork_enter.s - Enter ring 3 as a vfork() child.
;
; vfork_enter_user(frame) copies a saved registers_t (the parent's, with
; EAX = 0) onto this process's kernel stack and leaves through it with
; iret: the child continues right after the parent's int 0x80, in ring 3,
; on the parent's stack, believing vfork() returned 0.
; (The same as cpu_management's clone_enter.s, copied: each folder can be
; read on its own.)
; =============================================================================

global vfork_enter_user

FRAME_DWORDS equ 16             ; registers_t, lowest address first:
                                ; ds | edi esi ebp esp ebx edx ecx eax |
                                ; int_no err_code | eip cs eflags useresp ss

section .text
vfork_enter_user:
    cli                         ; nobody may interrupt a half-built frame
    mov esi, [esp + 4]          ; arg: the frame to copy

    sub esp, FRAME_DWORDS * 4   ; room for it on our own kernel stack
    mov edi, esp
    mov ecx, FRAME_DWORDS
    cld
    rep movsd

    pop eax                     ; ds: the user data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa                        ; edi esi ebp (esp skipped) ebx edx ecx eax
    add esp, 8                  ; drop int_no and err_code
    iret                        ; eip, cs, eflags, esp, ss -> ring 3
