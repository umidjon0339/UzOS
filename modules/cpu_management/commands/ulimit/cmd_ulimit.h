#ifndef CMD_ULIMIT_H
#define CMD_ULIMIT_H

/*
 * cmd_ulimit.h - `ulimit [-SH] [-a | -tfdsunerl [value]]`
 *
 * Show or set the shell's resource limits.
 */

void cmd_ulimit(const char *args);

#endif
