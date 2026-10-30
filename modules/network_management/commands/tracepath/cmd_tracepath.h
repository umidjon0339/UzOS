#ifndef CMD_TRACEPATH_H
#define CMD_TRACEPATH_H

/*
 * cmd_tracepath.h - `tracepath [-m hops] host`
 *
 * The path to a host and its MTU, without needing root.
 */

void cmd_tracepath(const char *args);

#endif
