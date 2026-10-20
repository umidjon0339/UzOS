#ifndef FD_H
#define FD_H

/*
 * fd.h - File descriptors: small numbers that stand for open files.
 *
 * A program never holds a pointer to a file. It holds a number, the
 * "file descriptor", and the kernel looks it up in the calling process's
 * table (process_t.fd_table). Every process starts with three:
 *
 *   0  STDIN   standard input   -> the keyboard
 *   1  STDOUT  standard output  -> the screen
 *   2  STDERR  standard error   -> the screen
 *
 * open() hands out the lowest free number from 3 upwards.
 */

#include <stdint.h>
#include "process.h"
#include "vfs.h"

#define STDIN         0
#define STDOUT        1
#define STDERR        2
#define FIRST_FILE_FD 3

#define IO_MAX_TRANSFER 16384u      // largest single read() or write()

// Put 'node' (opened with 'flags') in the lowest free slot of p's table.
// Returns the descriptor, or -1 if all MAX_FDS are in use.
int fd_alloc(process_t *p, vfs_node_t *node, uint32_t flags);

// Close descriptor 'fd' of p: forget it and lower the file's open count.
void fd_release(process_t *p, int fd);

// How many files p has open.
int fd_count(const process_t *p);

#endif
