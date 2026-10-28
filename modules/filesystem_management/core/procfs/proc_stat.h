#ifndef PROC_STAT_H
#define PROC_STAT_H

/*
 * proc_stat.h - The text of /proc/stat.
 */

#include "modules/filesystem_management/core/procfs/procfs.h"      // proc_buf_t

// Write the system-wide counters, in Linux's format.
void proc_stat(proc_buf_t *buf);

#endif
