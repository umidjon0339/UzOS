; =============================================================================
; switch_task.s - Swap the CPU from one process to another.
;
; void switch_task(uint32_t *save_esp, uint32_t load_esp);
;
; Every process that is not running sits inside a call to switch_task.
; Its registers are pushed on its own stack, and its ESP is saved in its
; process_t. To switch, we push our registers, save our ESP, load the
; other process's ESP, and pop ITS registers. The 'ret' then returns
; into the other process, exactly where it once called switch_task.
;
; Stack of a process that is not running (top = higher addresses):
;
;     return address      <- where it continues
;     EFLAGS              <- its interrupt flag (on or off)
;     EBP, EBX, ESI, EDI  <- registers C expects a call to keep
;                         <- saved ESP points here
;
; EFLAGS must be saved too. Otherwise a process that went to sleep with
; interrupts ON could wake up from inside the timer interrupt with them
; OFF, and the next 'hlt' would freeze the machine.
; =============================================================================

global switch_task

section .text
switch_task:
    pushfd                  ; save this process's EFLAGS
    push ebp                ; save the registers C needs kept
    push ebx
    push esi
    push edi

    mov eax, [esp + 24]     ; arg 1: save_esp (5 pushes = 20, + return address)
    mov ecx, [esp + 28]     ; arg 2: load_esp

    mov [eax], esp          ; remember where we stopped
    mov esp, ecx            ; jump onto the other process's stack

    pop edi                 ; restore ITS registers
    pop esi
    pop ebx
    pop ebp
    popfd                   ; and its EFLAGS
    ret                     ; continue where it stopped
