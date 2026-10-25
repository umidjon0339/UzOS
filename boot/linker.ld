/*
 * linker.ld - Memory layout of kernel.bin.
 *
 * The kernel is loaded at 1 MiB (below that is BIOS/VGA memory).
 * Each section starts on a 4 KiB page so paging can protect it later.
 * _kernel_start/_kernel_end tell the PMM which frames the kernel uses.
 * _text_* and _rodata_* mark the parts that never change after boot:
 * security_management/core/integrity hashes them.
 */
ENTRY(_start)

SECTIONS
{
    . = 1M;
    _kernel_start = .;

    /* Code. The multiboot header goes first so the bootloader finds it. */
    .text ALIGN(4K) :
    {
        _text_start = .;
        KEEP(*(.multiboot))
        *(.text .text.*)
        _text_end = .;
    }

    /* Constants and string literals. */
    .rodata ALIGN(4K) :
    {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    }

    /* Initialised global variables. */
    .data ALIGN(4K) :
    {
        *(.data .data.*)
    }

    /* Zeroed global variables and the boot stack. */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    . = ALIGN(4K);
    _kernel_end = .;

    /* Not needed in a kernel: unwind tables, notes, compiler comments. */
    /DISCARD/ :
    {
        *(.eh_frame .eh_frame_hdr)
        *(.note .note.*)
        *(.comment)
    }
}
