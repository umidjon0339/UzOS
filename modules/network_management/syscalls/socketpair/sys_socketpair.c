/*
 * sys_socketpair.c - socketpair(): two connected AF_UNIX sockets.
 *
 * What one end writes, the other reads, like a pipe that goes both
 * ways. A parent makes the pair, starts a child, and each keeps one end.
 *
 * Uses: socket_create / socket_install_fd / socket_close (core/socket),
 *       unix_pair (core/unix), scheduler, fd_release (io_management).
 */
#include "modules/network_management/syscalls/socketpair/sys_socketpair.h"  // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_create, socket_install_fd
#include "modules/network_management/core/unix/unix.h"                      // unix_pair
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process
#include "modules/io_management/core/fd/fd.h"                               // fd_release

int sys_socketpair(int domain, int type, int protocol, int sv[2]) {
    if (domain != AF_UNIX)
        return -NET_EAFNOSUPPORT;               // only local sockets can be paired
    socket_t *a, *b;
    int r = socket_create(domain, type, protocol, &a);
    if (r < 0)
        return r;
    if ((r = socket_create(domain, type, protocol, &b)) < 0) {
        socket_close(a);
        return r;
    }
    if ((r = unix_pair(a, b)) < 0) {
        socket_close(a);
        socket_close(b);
        return r;
    }
    process_t *self = scheduler_get_current_process();
    int fa = socket_install_fd(self, a);
    int fb = fa < 0 ? fa : socket_install_fd(self, b);
    if (fb < 0) {
        if (fa >= 0)
            fd_release(self, fa);               // closes a (release)
        else
            socket_close(a);
        socket_close(b);
        return -NET_EMFILE;
    }
    sv[0] = fa;
    sv[1] = fb;
    return 0;
}
