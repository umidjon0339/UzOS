#ifndef STRING_H
#define STRING_H

/*
 * string.h - Memory and string helpers for the kernel.
 *
 * The kernel has no C library (we build with -ffreestanding), so we
 * write the few functions we need ourselves. They behave like the
 * standard C versions with the same names.
 */

#include <stddef.h>

// Memory blocks
void *memset(void *dest, int value, size_t n);              // fill n bytes with value
void *memcpy(void *dest, const void *src, size_t n);        // copy n bytes (no overlap)

// Strings (NUL-terminated)
size_t strlen(const char *s);                               // length without the NUL
int    strcmp(const char *a, const char *b);                // <0, 0, >0 like the C library
int    strncmp(const char *a, const char *b, size_t n);     // the same, first n bytes only
char  *strcpy(char *dest, const char *src);                 // copy including the NUL
char  *strncpy(char *dest, const char *src, size_t n);      // copy at most n, pad with NUL
char  *strrchr(const char *s, int c);                       // last c in s, or NULL

#endif
