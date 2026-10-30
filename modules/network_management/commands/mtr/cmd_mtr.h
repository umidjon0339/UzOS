#ifndef CMD_MTR_H
#define CMD_MTR_H

/*
 * cmd_mtr.h - `mtr [-c cycles] host`
 *
 * traceroute and ping together: loss and timing of every hop.
 */

void cmd_mtr(const char *args);

#endif
