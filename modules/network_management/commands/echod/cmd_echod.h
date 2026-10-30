#ifndef CMD_ECHOD_H
#define CMD_ECHOD_H

/*
 * cmd_echod.h - `echod start [port] | stop | status`
 *
 * An echo server in the background: every byte comes straight back.
 */

void cmd_echod(const char *args);

#endif
