#ifndef CMD_LN_H
#define CMD_LN_H

/*
 * cmd_ln.h - `ln [-s] <target> <name>`
 *
 * Give a file another name (hard link), or make a symbolic link with -s.
 */

void cmd_ln(const char *args);

#endif
