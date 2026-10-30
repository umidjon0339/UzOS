#ifndef CMD_TRUNCATE_H
#define CMD_TRUNCATE_H

/*
 * cmd_truncate.h - `truncate -s SIZE <path>...`
 *
 * Make files exactly SIZE bytes: cut, or extend with zero bytes.
 */

void cmd_truncate(const char *args);

#endif
