/*
 * stdlib.c - atoi() and strtoul() for parsing shell arguments
 *            like "kill 5" or "pagewalk 0x1000".
 */

#include "stdlib.h"

int atoi(const char *s) {
    int sign = 1;
    int value = 0;

    while (*s == ' ')
        s++;

    if (*s == '-' || *s == '+') {
        if (*s == '-')
            sign = -1;
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }

    return sign * value;
}

static int digit_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 99;      // not a digit in any base we support
}

unsigned long strtoul(const char *s, char **end, int base) {
    unsigned long value = 0;

    while (*s == ' ')
        s++;

    int has_hex_prefix = s[0] == '0' && (s[1] == 'x' || s[1] == 'X');
    if ((base == 0 || base == 16) && has_hex_prefix) {
        base = 16;
        s += 2;
    } else if (base == 0) {
        base = 10;
    }

    while (digit_value(*s) < base) {
        value = value * base + digit_value(*s);
        s++;
    }

    if (end)
        *end = (char *)s;
    return value;
}
