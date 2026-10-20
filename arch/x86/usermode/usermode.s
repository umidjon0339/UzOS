; usermode.s - Enter ring 3 with a fake interrupt return.
;
; void switch_to_user_mode(uint32_t eip, uint32_t esp);
;
; 'iret' pops EIP, CS, EFLAGS, ESP, SS. If CS is a ring 3 selector,
; the CPU switches to ring 3. So we push exactly that frame.

global switch_to_user_mode

USER_CODE equ 0x1B          ; GDT slot 3 | ring 3
USER_DATA equ 0x23          ; GDT slot 4 | ring 3

EFLAGS_IF   equ 0x0200      ; interrupts enabled
EFLAGS_IOPL equ 0x3000      ; I/O privilege level bits
EFLAGS_NT   equ 0x4000      ; nested task (hardware task switching)

section .text
switch_to_user_mode:
    cli                     ; nothing may interrupt a half-built frame

    mov ebx, [esp + 4]      ; arg 1: user EIP
    mov ecx, [esp + 8]      ; arg 2: user ESP

    mov ax, USER_DATA       ; user data segments
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Build the iret frame (pushed in reverse order).
    push USER_DATA          ; SS
    push ecx                ; ESP

    pushf                   ; EFLAGS: start from the current value,
    pop eax
    and eax, ~(EFLAGS_IOPL | EFLAGS_NT)   ; IOPL 0: no in/out from ring 3
    or  eax, EFLAGS_IF                    ; interrupts on in user mode
    push eax

    push USER_CODE          ; CS
    push ebx                ; EIP

    iret                    ; -> ring 3
