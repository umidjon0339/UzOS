/*
 * sys_socketcall.c - How long each socketcall()'s argument array is.
 *
 * The dispatching itself is in the syscall handler (process_management/
 * syscalls/handler): it unpacks the array into registers and runs the
 * direct syscall's checked wrapper, so both doors check pointers alike.
 *
 * Uses: nothing.
 */
#include "modules/network_management/syscalls/socketcall/sys_socketcall.h"  // this file

int socketcall_arg_count(int call) {
    // Linux's nargs[] table: arguments of call 1 .. 20.
    static const uint8_t counts[] = {
        0, 3, 3, 3, 2, 3, 3, 3, 4, 4, 4, 6, 6, 2, 5, 5, 3, 3, 4, 5, 4,
    };
    return call >= 1 && call <= 20 ? counts[call] : 0;
}
