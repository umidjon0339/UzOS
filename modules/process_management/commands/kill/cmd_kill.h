#ifndef CMD_KILL_H
#define CMD_KILL_H

/*
 * cmd_kill.h - `kill <pid>`
 *
 * Stop a process (kill_guard inside process_kill decides who may).
 */

void cmd_kill(const char *args);

#endif
