; =============================================================================
; clone3_enter.s - Start a clone3()d thread in ring 3.
;
; void clone3_enter_user(const registers_t *frame);
;
; 'frame' is a copy of the registers the parent had when it called
; clone3() (with EAX = 0 and ESP = the new stack). An interrupt ends by
; popping exactly such a frame and running 'iret' (arch/x86/isr/
; isr_stubs.s); we do the same, so the child "returns from clone3()".
;
; registers_t, lowest address first (16 dwords = 64 bytes):
;   ds | edi esi ebp esp ebx edx ecx eax | int_no err_code | eip cs eflags useresp ss
; =============================================================================

global clone3_enter_user

FRAME_DWORDS equ 16

section .text
clone3_enter_user:
    cli                         ; nobody may interrupt a half-built frame
    mov esi, [esp + 4]          ; arg: the frame to copy

    sub esp, FRAME_DWORDS * 4   ; room for it on our own kernel stack
    mov edi, esp
    mov ecx, FRAME_DWORDS
    cld
    rep movsd                   ; copy it

    pop eax                     ; ds: the user data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa                        ; edi esi ebp (esp skipped) ebx edx ecx eax
    add esp, 8                  ; drop int_no and err_code
    iret                        ; eip, cs, eflags, esp, ss -> ring 3
