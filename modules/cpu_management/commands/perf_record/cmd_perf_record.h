#ifndef CMD_PERF_RECORD_H
#define CMD_PERF_RECORD_H

/*
 * cmd_perf_record.h - `perf record [-o FILE] command`
 *
 * Sample where the CPU spends its time while a command runs.
 */

void cmd_perf_record(const char *args);

#endif
