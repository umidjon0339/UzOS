#ifndef CMD_ARPING_H
#define CMD_ARPING_H

/*
 * cmd_arping.h - `arping [-c count] [-I dev] IP`
 *
 * Ask "who has IP?" on the link and time each answer.
 */

void cmd_arping(const char *args);

#endif
