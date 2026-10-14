#include "../proc_commands_internal.h"
#include "proc_commands.h"
#include "shell.h"

void register_proc_commands(void) {
    cmd_ps_register();
    shell_register_command("top", "Live view of processes", cmd_top);
    shell_register_command("kill", "Kill a process by PID", cmd_kill);
    shell_register_command("spawn", "Spawn a background worker thread", cmd_spawn);
    shell_register_command("sleep", "Sleep for N seconds", cmd_sleep);
    shell_register_command("exec", "Execute an ELF binary from the filesystem", cmd_exec);
    shell_register_command("testuser", "Test Ring 3 Switch & Syscall (WARNING: Hangs thread)", cmd_testuser);
    shell_register_command("seclab", "Process security lab: attacks and defences", cmd_seclab);
}
