#ifndef CMD_PS_H
#define CMD_PS_H

#define CMD_PS_NAME "ps"
#define CMD_PS_DESC "List running processes"

void cmd_ps(const char *args);
void cmd_ps_register(void);

#endif
