#ifndef CMD_STAT_H
#define CMD_STAT_H

/*
 * cmd_stat.h - `stat <path>...`
 *
 * Everything about a file but its bytes: inode, links, owner, rights, times.
 */

void cmd_stat(const char *args);

#endif
