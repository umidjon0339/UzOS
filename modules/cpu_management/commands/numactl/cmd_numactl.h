#ifndef CMD_NUMACTL_H
#define CMD_NUMACTL_H

/*
 * cmd_numactl.h - `numactl --hardware | --show | [policy options] command`
 *
 * NUMA nodes, and run a command with a memory / CPU placement.
 */

void cmd_numactl(const char *args);

#endif
