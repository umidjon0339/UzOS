#ifndef SYS_SOCKETCALL_H
#define SYS_SOCKETCALL_H
/*
 * sys_socketcall.h - socketcall(call, args): every socket call through one door.
 *
 * Old 32-bit Linux had a single syscall for all socket operations: EBX
 * says which one, ECX points at an array with its arguments. (Since
 * Linux 4.3 each also has its own number, as in UzOS.) The handler
 * reads the array and runs the same checked call as the direct syscall.
 */
#include <stdint.h>
#define SYS_SOCKETCALL 149  // EAX=149, EBX=call (SOCKETCALL_*), ECX=args (uint32_t[])

// The calls, numbered as in Linux's <linux/net.h>
#define SOCKETCALL_SOCKET      1
#define SOCKETCALL_BIND        2
#define SOCKETCALL_CONNECT     3
#define SOCKETCALL_LISTEN      4
#define SOCKETCALL_ACCEPT      5
#define SOCKETCALL_GETSOCKNAME 6
#define SOCKETCALL_GETPEERNAME 7
#define SOCKETCALL_SOCKETPAIR  8
#define SOCKETCALL_SEND        9
#define SOCKETCALL_RECV        10
#define SOCKETCALL_SENDTO      11
#define SOCKETCALL_RECVFROM    12
#define SOCKETCALL_SHUTDOWN    13
#define SOCKETCALL_SETSOCKOPT  14
#define SOCKETCALL_GETSOCKOPT  15
#define SOCKETCALL_SENDMSG     16
#define SOCKETCALL_RECVMSG     17
#define SOCKETCALL_ACCEPT4     18
#define SOCKETCALL_RECVMMSG    19
#define SOCKETCALL_SENDMMSG    20

// How many arguments 'call' takes (so the handler checks exactly that
// much of the array), or 0 if there is no such call.
int socketcall_arg_count(int call);
#endif
