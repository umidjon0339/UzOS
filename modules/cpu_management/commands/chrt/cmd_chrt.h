#ifndef CMD_CHRT_H
#define CMD_CHRT_H

/*
 * cmd_chrt.h - `chrt [-p] [-f|-r|-o|-b|-i] [-R] [prio] [PID | command] | chrt -m`
 *
 * Show or set the scheduling policy and real-time priority.
 */

void cmd_chrt(const char *args);

#endif
