#include "../mem_commands_internal.h"
#include "mem_commands.h"
#include "shell.h"

void register_mem_commands(void) {
    shell_register_command("meminfo",  "Show physical memory status", cmd_meminfo);
    shell_register_command("heapinfo", "Show kernel heap status",     cmd_heapinfo);
}
