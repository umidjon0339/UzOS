#ifndef IO_SECURITY_H
#define IO_SECURITY_H

#include "process.h"

#define IO_STDIN  0
#define IO_STDOUT 1
#define IO_STDERR 2
#define IO_FIRST_FILE_FD 3
#define IO_MAX_TRANSFER 16384u

int io_fd_index_valid(int fd);
int io_file_descriptor_valid(const process_t *proc, int fd);
int io_buffer_valid(const void *buffer, uint32_t count);

#endif
