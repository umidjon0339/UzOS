/*
 * mem_commands.c - Register the memory commands with the shell.
 *
 * Uses: shell_register_command (shell).
 */

#include "mem_commands.h"
#include "shell.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "meminfo",  "Physical RAM: frames used/free, memory map", cmd_meminfo  },
    { "heapinfo", "Kernel heap: blocks, bytes, health",         cmd_heapinfo },
    { "pagewalk", "Translate a virtual address: pagewalk <addr>", cmd_pagewalk },
    { "memtest",  "Try sbrk, mmap and munmap and check them",   cmd_memtest  },
};

void register_mem_commands(void) {
    shell_begin_group("Memory");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}
