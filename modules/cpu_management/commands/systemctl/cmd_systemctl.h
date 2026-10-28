#ifndef CMD_SYSTEMCTL_H
#define CMD_SYSTEMCTL_H

/*
 * cmd_systemctl.h - `systemctl [list-units | status UNIT | show UNIT]`
 *
 * Services (the system processes) and their resource settings.
 */

void cmd_systemctl(const char *args);

#endif
