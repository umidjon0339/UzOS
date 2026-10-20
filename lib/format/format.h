#ifndef FORMAT_H
#define FORMAT_H

/*
 * format.h - Write numbers and text into a string (a small snprintf).
 *
 *     char line[40];
 *     format(line, sizeof(line), "%s: %d MB (%3u%%)", "RAM", 127, 7);
 *     // "RAM: 127 MB (  7%)"
 *
 * Supported:   %s  %c  %d  %u  %x  %%
 * Width:       %5d  pads with spaces on the left    "   42"
 *              %-5d pads on the right               "42   "
 *              %05d pads with zeros                 "00042"
 */

#include <stdarg.h>

// Never writes more than 'size' bytes and always ends 'out' with a NUL.
// Returns the length of the text written.
int format(char *out, int size, const char *fmt, ...);
int vformat(char *out, int size, const char *fmt, va_list args);

#endif
