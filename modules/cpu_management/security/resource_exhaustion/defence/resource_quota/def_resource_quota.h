#ifndef DEF_RESOURCE_QUOTA_H
#define DEF_RESOURCE_QUOTA_H

// 1 if every resource a process can exhaust in UzOS (CPU time, process
// slots, open files, file size, heap) carries a limit the kernel
// enforces. The limits live in core/rlimit; this is the roll-call.
int def_resource_quota_complete(void);

#endif
