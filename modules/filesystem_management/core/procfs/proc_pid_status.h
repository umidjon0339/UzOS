#ifndef PROC_PID_STATUS_H
#define PROC_PID_STATUS_H

/*
 * proc_pid_status.h - The text of /proc/<pid>/status.
 */

#include "modules/filesystem_management/core/procfs/procfs.h"  // proc_buf_t, process_t

// Write the status of process 'p', in Linux's format.
void proc_pid_status(proc_buf_t *buf, const process_t *p);

#endif
