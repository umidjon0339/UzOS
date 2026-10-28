#ifndef CMD_GETCONF_H
#define CMD_GETCONF_H

/*
 * cmd_getconf.h - `getconf NAME | -a`
 *
 * Print system configuration values (sysconf / confstr).
 */

void cmd_getconf(const char *args);

#endif
