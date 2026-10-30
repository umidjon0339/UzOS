#ifndef PROCFS_H
#define PROCFS_H

/*
 * procfs.h - /proc: files the kernel writes the moment you read them.
 *
 * Nothing in /proc is stored anywhere. When `cat /proc/stat` reads, the
 * VFS calls this driver's read function, which builds the text from
 * the kernel's live counters right then. That is how Linux shows its
 * insides to programs: as files, so every tool can read them.
 *
 *   /proc/
 *   ├── cpuinfo          the CPU             (proc_cpuinfo.c)
 *   ├── stat             CPU time, counters  (proc_stat.c)
 *   ├── net/             the network: dev arp route tcp udp unix snmp (proc_net.c)
 *   ├── self/            the process reading it
 *   │   └── status
 *   └── <pid>/           one directory per process, made up on lookup
 *       └── status       its state, ids, CPU mask, switches (proc_pid_status.c)
 *
 * Like the RAM disk (core/ramdisk), this is a filesystem DRIVER, so it
 * lives in filesystem_management. The numbers it shows come from
 * cpu_management/core (accounting, affinity, numa, ...).
 *
 * The directories <pid>/ are not stored either: looking up "7" asks the
 * process table whether PID 7 exists, and `ls /proc` lists whoever
 * exists right now.
 */

#include <stdint.h>
#include "modules/filesystem_management/core/vfs/vfs.h"       // vfs_node_t
#include "modules/process_management/core/process/process.h"  // process_t

#define PROC_TEXT_MAX 4096      // the longest file /proc makes

// Where a generator writes its text.
typedef struct {
    char *text;
    int   length;
    int   size;
} proc_buf_t;

// printf into a proc buffer (stops quietly when it is full).
void proc_printf(proc_buf_t *buf, const char *fmt, ...);

// Make /proc and hang it into the root directory. Called once from
// kernel_main, after the RAM disk exists.
void procfs_init(void);

#endif
