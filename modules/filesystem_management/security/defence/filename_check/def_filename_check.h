#ifndef DEF_FILENAME_CHECK_H
#define DEF_FILENAME_CHECK_H

// Is a path component of 'len' characters short enough to copy? (logged if not)
int def_component_ok(int len);

// May a file or directory be created/renamed to 'name'? (logged if not)
int def_new_name_ok(const char *name);

#endif
