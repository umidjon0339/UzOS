#ifndef CMD_UMASK_H
#define CMD_UMASK_H

/*
 * cmd_umask.h - `umask [mask]`
 *
 * Show or set the mode bits new files leave out (Linux octal: 022).
 */

void cmd_umask(const char *args);

#endif
