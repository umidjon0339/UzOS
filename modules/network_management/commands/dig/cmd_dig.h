#ifndef CMD_DIG_H
#define CMD_DIG_H

/*
 * cmd_dig.h - `dig [@server] name [type] [+short]`
 *
 * The whole DNS answer: header, question, records with their TTLs.
 */

void cmd_dig(const char *args);

#endif
