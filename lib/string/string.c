/*
 * string.c - Simple byte-by-byte versions of the standard C helpers.
 *
 * GCC may also call memset/memcpy on its own (e.g. to zero a struct),
 * so these must exist even if no kernel code calls them directly.
 */

#include "string.h"

void *memset(void *dest, int value, size_t n) {
    unsigned char *d = dest;

    while (n--)
        *d++ = (unsigned char)value;

    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;

    while (n--)
        *d++ = *s++;

    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;

    while (s[len])
        len++;

    return len;
}

int strcmp(const char *a, const char *b) {
    // Skip the common prefix, then compare the first different byte.
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;

    while ((*d++ = *src++))
        ;

    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;

    // Copy up to n characters...
    for (; i < n && src[i]; i++)
        dest[i] = src[i];

    // ...and fill the rest with NULs. (If src is n long or more,
    // dest is NOT terminated, same as standard strncpy.)
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

char *strrchr(const char *s, int c) {
    const char *last = 0;
    for (; *s; s++)
        if (*s == (char)c)
            last = s;
    return (char *)last;
}
