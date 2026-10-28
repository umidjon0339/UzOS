#ifndef CMD_PS_H
#define CMD_PS_H

/*
 * cmd_ps.h - `ps [-e|-A] [-f] [-L] [aux] [-o FIELDS] [-p PIDS] [-u USER] [--sort=[-]FIELD]`
 *
 * Report a snapshot of the processes, with their CPU use.
 */

void cmd_ps(const char *args);

#endif
