#include "io_commands.h"
#include "../io_commands_internal.h"
#include "shell.h"

void register_io_commands(void) {
    shell_register_command("ioinfo", "Show standard streams and open file descriptors",
                           cmd_ioinfo);
}
