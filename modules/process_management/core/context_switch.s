global switch_task
global task_start_wrapper

section .text

; void switch_task(uint32_t *old_esp, uint32_t new_esp)
switch_task:
    ; Standard C calling convention:
    ; [esp + 4] = old_esp (pointer to where to save current esp)
    ; [esp + 8] = new_esp (the value to load into esp)
    
    ; Save caller-saved registers
    push ebp
    push ebx
    push esi
    push edi

    ; Get arguments
    mov eax, [esp + 20] ; old_esp
    mov ecx, [esp + 24] ; new_esp

    ; Save current esp
    mov [eax], esp

    ; Load new esp
    mov esp, ecx

    ; Restore registers from the new stack
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret

; wrapper for newly created tasks
task_start_wrapper:
    sti     ; Enable interrupts (since we bypassed iret)
    ret     ; Return to the entry point (which is the next item on the stack)
