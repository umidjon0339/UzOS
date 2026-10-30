/*
 * sys_sendfile.c - sendfile(): the bytes never visit the program.
 *
 * A web server sending a file would read() it into its own memory and
 * write() it to the socket: two copies across the kernel border.
 * sendfile asks the kernel to do both halves itself. With 'offset'
 * given, it reads from there (and moves *offset); without, from the
 * file's own position (and moves that).
 *
 * Uses: vfs_read / vfs_write (filesystem_management), socket_from_fd /
 *       socket_sendto (core/socket), scheduler.
 */
#include "modules/network_management/syscalls/sendfile/sys_sendfile.h"      // this file
#include "modules/network_management/core/socket/socket.h"                  // socket_from_fd, socket_sendto
#include "modules/network_management/core/inet/inet.h"                      // NET_E*
#include "modules/filesystem_management/core/vfs/vfs.h"                     // vfs_read, vfs_write
#include "modules/process_management/core/scheduler/scheduler.h"            // scheduler_get_current_process

#define CHUNK 2048

int sys_sendfile(int out_fd, int in_fd, uint32_t *offset, uint32_t count) {
    process_t *self = scheduler_get_current_process();
    // From a file (3 and up) to a socket or another file (not the console).
    if (in_fd < 3 || in_fd >= MAX_FDS || out_fd < 3 || out_fd >= MAX_FDS ||
        !self->fd_table[in_fd] || !self->fd_table[out_fd])
        return -NET_EBADF;
    vfs_node_t *in = self->fd_table[in_fd];
    if (socket_from_fd(self, in_fd) || !(in->flags & FS_FILE))
        return -NET_EINVAL;                     // the source must be a file
    socket_t *out = socket_from_fd(self, out_fd);

    uint32_t pos = offset ? *offset : self->fd_offset[in_fd];
    uint32_t done = 0;
    static uint8_t chunk[CHUNK];
    while (done < count) {
        uint32_t want = count - done < CHUNK ? count - done : CHUNK;
        uint32_t got = vfs_read(in, pos + done, want, chunk);
        if (!got)
            break;                              // end of the file
        int sent = out ? socket_sendto(out, chunk, got, 0, 0, 0)
                       : (int)vfs_write(self->fd_table[out_fd], self->fd_offset[out_fd], got, chunk);
        if (sent <= 0) {
            if (!done)
                return sent < 0 ? sent : -NET_EINVAL;
            break;
        }
        if (!out)
            self->fd_offset[out_fd] += (uint32_t)sent;
        done += (uint32_t)sent;
        if ((uint32_t)sent < got)
            break;
    }
    if (offset)
        *offset = pos + done;
    else
        self->fd_offset[in_fd] = pos + done;
    return (int)done;
}
