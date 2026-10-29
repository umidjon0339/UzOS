#ifndef DEF_SIGNAL_PERMISSION_H
#define DEF_SIGNAL_PERMISSION_H

#include <stdint.h>

// May 'caller_uid' send a signal to a process owned by 'target_uid'?
// System processes (part of the kernel) may never be signalled; a
// holder of CAP_KILL may signal any other; otherwise you may signal only
// your own processes. The kill / tkill / tgkill paths ask this before
// delivering a signal. 1 = allowed, 0 = refused (logged).
int def_signal_allowed(uint32_t caller_uid, uint32_t target_uid, int target_is_system);

#endif
