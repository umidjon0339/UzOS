/*
 * args.c - arg_next(): take one word off the front of a string.
 */

#include "args.h"

const char *arg_next(const char *s, char *out, int max) {
    int len = 0;

    while (*s == ' ')
        s++;
    while (*s && *s != ' ') {
        if (len < max - 1)
            out[len++] = *s;
        s++;                    // a too-long word is cut, never overflows
    }
    out[len] = '\0';

    while (*s == ' ')
        s++;
    return s;
}
