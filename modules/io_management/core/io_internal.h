#ifndef IO_INTERNAL_H
#define IO_INTERNAL_H

#include "process.h"

int io_find_free_file_descriptor(const process_t *proc);

#endif
