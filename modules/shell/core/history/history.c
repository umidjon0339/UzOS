/*
 * history.c - Store and browse past commands.
 *
 * Uses: strcmp / strncpy (lib/string).
 */

#include "history.h"
#include "line.h"
#include "string.h"

static char entries[HISTORY_SIZE][LINE_MAX];
static int newest;              // index of the next free slot
static int count;               // entries stored (at most HISTORY_SIZE)
static int steps_back;          // 0 = not browsing, 1 = newest, 2 = the one before...

// The entry 'back' steps before the newest (1 = newest).
static const char *entry_back(int back) {
    return entries[(newest - back + HISTORY_SIZE) % HISTORY_SIZE];
}

void history_add(const char *line) {
    if (!line[0] || (count && strcmp(entry_back(1), line) == 0))
        return;

    strncpy(entries[newest], line, LINE_MAX - 1);
    entries[newest][LINE_MAX - 1] = '\0';
    newest = (newest + 1) % HISTORY_SIZE;
    if (count < HISTORY_SIZE)
        count++;
}

const char *history_older(void) {
    if (steps_back >= count)
        return 0;               // already at the oldest
    return entry_back(++steps_back);
}

const char *history_newer(void) {
    if (steps_back <= 1) {
        steps_back = 0;
        return 0;               // past the newest: back to an empty line
    }
    return entry_back(--steps_back);
}

void history_stop_browsing(void) {
    steps_back = 0;
}

int history_browsing(void) {
    return steps_back > 0;
}

int history_count(void) {
    return count;
}

const char *history_get(int index) {
    if (index < 0 || index >= count)
        return 0;
    return entry_back(count - index);
}
