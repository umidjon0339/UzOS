#ifndef MULTIBOOT_H
#define MULTIBOOT_H

/*
 * multiboot.h - What the bootloader tells the kernel.
 *
 * boot.s passes EBX (a multiboot_info_t*) to kernel_main(). The PMM reads
 * the memory map from it to learn which RAM is free.
 * Spec: https://www.gnu.org/software/grub/manual/multiboot/
 */

#include <stdint.h>

// EAX holds this value if a multiboot bootloader started us.
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// Bits in multiboot_info_t.flags: which fields are valid.
#define MULTIBOOT_INFO_MEMORY   0x00000001  // mem_lower / mem_upper
#define MULTIBOOT_INFO_BOOTDEV  0x00000002  // boot_device
#define MULTIBOOT_INFO_CMDLINE  0x00000004  // cmdline
#define MULTIBOOT_INFO_MODS     0x00000008  // mods_count / mods_addr
#define MULTIBOOT_INFO_MEM_MAP  0x00000040  // mmap_length / mmap_addr

// One region of physical memory in the memory map.
typedef struct multiboot_mmap_entry {
    uint32_t size;      // size of this entry, NOT counting this field
    uint64_t addr;      // start address
    uint64_t len;       // length in bytes
    uint32_t type;      // MULTIBOOT_MEMORY_* below
} __attribute__((packed)) multiboot_memory_map_t;

// One file the bootloader loaded next to the kernel (mods_addr points
// to an array of mods_count of these).
typedef struct multiboot_module {
    uint32_t mod_start;     // first byte
    uint32_t mod_end;       // one past the last byte
    uint32_t string;        // its command line (char*)
    uint32_t reserved;
} __attribute__((packed)) multiboot_module_t;

// Memory region types
#define MULTIBOOT_MEMORY_AVAILABLE        1   // free RAM we may use
#define MULTIBOOT_MEMORY_RESERVED         2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE 3
#define MULTIBOOT_MEMORY_NVS              4
#define MULTIBOOT_MEMORY_BADRAM           5

// The info block the bootloader fills in.
typedef struct multiboot_info {
    uint32_t flags;             // which fields below are valid
    uint32_t mem_lower;         // KiB of RAM below 1 MiB
    uint32_t mem_upper;         // KiB of RAM above 1 MiB
    uint32_t boot_device;
    uint32_t cmdline;           // kernel command line (char*)
    uint32_t mods_count;        // number of loaded modules
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;       // memory map size in bytes
    uint32_t mmap_addr;         // first multiboot_memory_map_t
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;  // bootloader name (char*)
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot_info_t;

#endif
