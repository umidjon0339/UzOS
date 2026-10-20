#ifndef HISTORY_H
#define HISTORY_H

/*
 * history.h - The last commands typed, for the Up / Down keys.
 *
 * A ring of HISTORY_SIZE lines: when it is full, the oldest is dropped.
 * "Browsing" walks back from the newest; Down walks forward again.
 */

#define HISTORY_SIZE 16

// Remember a command (empty lines and repeats of the last one are skipped).
void history_add(const char *line);

// One step back / forward while browsing. NULL means: nothing older /
// back at the empty new line.
const char *history_older(void);
const char *history_newer(void);

// Stop browsing (the next Up starts from the newest again).
void history_stop_browsing(void);

// Is the user walking through history right now?
int history_browsing(void);

// For the `history` command: entry 0 is the oldest.
int history_count(void);
const char *history_get(int index);

#endif
