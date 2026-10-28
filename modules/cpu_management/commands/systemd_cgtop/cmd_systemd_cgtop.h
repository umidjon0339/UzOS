#ifndef CMD_SYSTEMD_CGTOP_H
#define CMD_SYSTEMD_CGTOP_H

/*
 * cmd_systemd_cgtop.h - `systemd-cgtop [-n COUNT]`
 *
 * CPU and memory per control group (UzOS: per slice of users).
 */

void cmd_systemd_cgtop(const char *args);

#endif
