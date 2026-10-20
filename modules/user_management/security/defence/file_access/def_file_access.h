#ifndef DEF_FILE_ACCESS_H
#define DEF_FILE_ACCESS_H

#include <stdint.h>

// May user 'uid' do 'want' (PERM_READ / PERM_WRITE) to a file owned by
// 'owner' with 'mode'? 1 = yes, 0 = refused (logged).
int def_file_access_ok(uint32_t uid, uint32_t owner, uint32_t mode, int want);

// Is 'uid' the owner (or root)? 1 = yes, 0 = refused (logged).
int def_file_access_owner_ok(uint32_t uid, uint32_t owner);

#endif
