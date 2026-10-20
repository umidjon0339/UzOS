#ifndef STATUSBAR_H
#define STATUSBAR_H

/*
 * statusbar.h - The bar on the top row of the screen.
 *
 *  ☾ UzOS │ /docs                    ⚙ 5 │ ▣ ██░░ 7% │ ◷ 14:32:05
 *
 * It is kept up to date by "statusd", a small kernel process (a
 * "daemon": it runs in the background, nobody calls it). Twice a second
 * it reads the clock, the memory and the process table, builds the row,
 * and hands it to the console (vga_status_set).
 *
 * The current directory shown is the shell's: the daemon looks at the
 * process the shell runs in.
 */

#include <stdint.h>

// Turn the bar on and start statusd. 'shell_pid' = the shell's process.
void statusbar_start(uint32_t shell_pid);

// Show or hide the bar (the daemon keeps running, but draws nothing).
void statusbar_show(int on);
int statusbar_shown(void);

// Build the bar and show it right now.
void statusbar_update(void);

#endif
