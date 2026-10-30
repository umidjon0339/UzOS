#ifndef SYS_PRCTL_H
#define SYS_PRCTL_H
/* sys_prctl.h - prctl(option, arg2): settings of the calling process. */
#include <stdint.h>
#define SYS_PRCTL 80            // EAX=80, EBX=option, ECX=arg2

// The options UzOS has (the same numbers as Linux).
#define PR_SET_PDEATHSIG        1   // arg2 = signal to get when the parent ends (0 = none)
#define PR_GET_PDEATHSIG        2   // arg2 = int * to fill
#define PR_GET_DUMPABLE         3   // returns 0 or 1
#define PR_SET_DUMPABLE         4   // arg2 = 0 or 1
#define PR_SET_NAME            15   // arg2 = name (16 bytes at most, with the NUL)
#define PR_GET_NAME            16   // arg2 = 16-byte buffer to fill
#define PR_SET_CHILD_SUBREAPER 36   // arg2 = 1: orphaned descendants come to us
#define PR_GET_CHILD_SUBREAPER 37   // arg2 = int * to fill
#define PR_SET_NO_NEW_PRIVS    38   // arg2 must be 1: for good
#define PR_GET_NO_NEW_PRIVS    39   // returns 0 or 1

#define PR_NAME_LEN 16          // TASK_COMM_LEN in Linux

int sys_prctl(int option, uint32_t arg2);
#endif
