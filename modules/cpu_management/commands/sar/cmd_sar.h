#ifndef CMD_SAR_H
#define CMD_SAR_H

/*
 * cmd_sar.h - `sar [-u|-q|-w] [interval [count]]`
 *
 * System activity: live, or the last minute from the kernel history.
 */

void cmd_sar(const char *args);

#endif
