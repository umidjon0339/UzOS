#ifndef DEF_HIDEPID_H
#define DEF_HIDEPID_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

// May user 'viewer_uid' see the DETAILS of 'target': its directory,
// open files, signals, memory? Its own (if it is dumpable), or anyone's
// with CAP_PTRACE. Everyone may still see that it exists (pid, name,
// owner). procinfo, pwdx and /proc/<pid>/status ask this.
// 1 = visible, 0 = hidden (logged).
int def_hidepid_visible(uint32_t viewer_uid, const process_t *target);

#endif
