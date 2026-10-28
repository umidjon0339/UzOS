#ifndef PROC_CPUINFO_H
#define PROC_CPUINFO_H

/*
 * proc_cpuinfo.h - The text of /proc/cpuinfo.
 */

#include "modules/filesystem_management/core/procfs/procfs.h"      // proc_buf_t

// Write one "processor" block per CPU (UzOS: one), in Linux's format.
void proc_cpuinfo(proc_buf_t *buf);

#endif
