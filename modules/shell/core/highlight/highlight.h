#ifndef HIGHLIGHT_H
#define HIGHLIGHT_H

/*
 * highlight.h - Colour the line while it is typed (like the fish shell).
 *
 * Only the first word, the command, gets a colour:
 *
 *     ls          green    a real command: Enter will run it
 *     l           normal   not a command yet, but "ls" starts like this
 *     lx          red      no command starts like this: a typo
 *
 * The rest of the line keeps the normal text colour.
 */

#include "vga.h"

// The colour for a command word of 'len' characters at 'word'.
enum vga_color highlight_command_color(const char *word, int len);

// Print a typed line with its command word coloured. Leaves the normal
// text colour on afterwards.
void highlight_print(const char *text);

#endif
