#ifndef CMD_PMAP_H
#define CMD_PMAP_H

/*
 * cmd_pmap.h - `pmap [pid]`
 *
 * The memory a process can reach: its kernel stack and the user pages.
 */

void cmd_pmap(const char *args);

#endif
