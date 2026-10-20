; gdt_load.s - Load a new GDT and reload every segment register.
;
; void gdt_load(gdt_ptr_t *ptr);
;
; After 'lgdt' the CPU still uses the old cached segments, so we must
; reload DS..SS with a mov, and CS with a far jump.

global gdt_load

KERNEL_CODE equ 0x08
KERNEL_DATA equ 0x10

section .text
gdt_load:
    mov eax, [esp + 4]      ; arg: pointer to gdt_ptr_t
    lgdt [eax]

    mov ax, KERNEL_DATA     ; data segments -> kernel data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp KERNEL_CODE:.reload_cs  ; far jump sets CS
.reload_cs:
    ret
