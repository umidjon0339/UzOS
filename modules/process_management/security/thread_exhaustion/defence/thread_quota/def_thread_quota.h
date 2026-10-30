#ifndef DEF_THREAD_QUOTA_H
#define DEF_THREAD_QUOTA_H

#include "modules/process_management/core/process/process.h"   // process_t

#define THREADS_PER_GROUP 4     // a program (thread group) has at most this many threads

// May 'parent' start one more thread (clone CLONE_THREAD) in its thread
// group? Not if the group already has THREADS_PER_GROUP live threads -
// for root too. clone() and clone3() (cpu_management) ask this.
// 1 = allowed, 0 = refused (logged).
int def_thread_quota_ok(const process_t *parent);

#endif
