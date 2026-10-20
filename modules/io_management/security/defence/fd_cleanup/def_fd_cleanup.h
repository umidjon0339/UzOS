#ifndef DEF_FD_CLEANUP_H
#define DEF_FD_CLEANUP_H

#include "process.h"

// Close every file process p still has open. Called when p ends.
void def_fd_close_all(process_t *p);

#endif
