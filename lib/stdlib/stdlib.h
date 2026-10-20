#ifndef STDLIB_H
#define STDLIB_H

/*
 * stdlib.h - Number parsing for the kernel (like the C library's).
 */

// Read a decimal number: skips leading spaces, allows one '+' or '-',
// stops at the first non-digit. "42abc" -> 42, "abc" -> 0.
int atoi(const char *s);

// Read an unsigned number in 'base' (10 or 16). Base 0 picks for you:
// "0x1F" is hex, "31" is decimal. If 'end' is not NULL it gets a pointer
// to the first character that was not part of the number.
unsigned long strtoul(const char *s, char **end, int base);

#endif
