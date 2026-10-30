#ifndef SYS_PIDFD_SEND_SIGNAL_H
#define SYS_PIDFD_SEND_SIGNAL_H
/* sys_pidfd_send_signal.h - signal the process a pidfd names. */
#include <stdint.h>
#define SYS_PIDFD_SEND_SIGNAL 61    // EAX=61, EBX=pidfd, ECX=sig
int sys_pidfd_send_signal(int pidfd, int sig);
#endif
