ENTRY(start)

SECTIONS
{
    /* 
     * Multiboot2 expects the kernel to be loaded at 1MB (0x100000).
     * This is standard for x86 kernels. 
     */
    . = 1M;

    /* 
     * .text section contains the executable code.
     * We ensure the multiboot header comes first by using KEEP().
     * The ALIGN(4K) ensures that the section starts at a page boundary.
     */
    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot_header))
        *(.text)
    }

    /* .rodata section contains read-only data, such as strings. */
    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }

    /* .data section contains initialized read-write data. */
    .data ALIGN(4K) :
    {
        *(.data)
    }

    /* 
     * .bss section contains uninitialized data and stack. 
     * It occupies no space in the file on disk. 
     */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }
}
