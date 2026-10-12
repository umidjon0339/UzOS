#include "../fs_commands_internal.h"
#include "fs_commands.h"
#include "shell.h"

void register_fs_commands(void) {
    shell_register_command("ls", "List files and directories", cmd_ls);
    shell_register_command("mkdir", "Create a directory", cmd_mkdir);
    shell_register_command("touch", "Create an empty file", cmd_touch);
    shell_register_command("write", "Write text to a file", cmd_write);
    shell_register_command("cat", "Print file contents", cmd_cat);
    shell_register_command("rm", "Delete a file or directory", cmd_rm);
    shell_register_command("cd", "Change working directory", cmd_cd);
    shell_register_command("pwd", "Print working directory", cmd_pwd);
    shell_register_command("cp", "Copy a file", cmd_cp);
    shell_register_command("mv", "Move/rename a file", cmd_mv);
    shell_register_command("wc", "Word, line, and character count", cmd_wc);
    shell_register_command("grep", "Search for pattern in file", cmd_grep);
    shell_register_command("hexdump", "Display hex and ASCII dump of file", cmd_hexdump);
}
