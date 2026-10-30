#ifndef CMD_TESTUSER_H
#define CMD_TESTUSER_H

/*
 * cmd_testuser.h - `testuser [null|badptr]`
 *
 * Run a tiny program in ring 3 through usertask.
 */

void cmd_testuser(const char *args);

#endif
