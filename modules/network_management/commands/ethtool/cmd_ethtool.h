#ifndef CMD_ETHTOOL_H
#define CMD_ETHTOOL_H

/*
 * cmd_ethtool.h - `ethtool [-i | -S | -d | -t] [eth0]`
 *
 * The network card: link, speed, driver, counters, registers, self-test.
 */

void cmd_ethtool(const char *args);

#endif
