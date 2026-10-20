#ifndef COMPLETE_H
#define COMPLETE_H

/*
 * complete.h - Tab completion.
 *
 * The first word is completed from the registered commands, any later
 * word from the files in its directory ("docs/ol" -> "docs/old/").
 * One match: finish the word. Several: add what they all share, or
 * list them if there is nothing more to add.
 */

#include "line.h"

typedef enum {
    COMPLETE_NOTHING,       // no match, line unchanged
    COMPLETE_ADDED,         // characters were added: echo text[old_length..]
    COMPLETE_LISTED,        // matches were printed: redraw prompt + line
} complete_result_t;

complete_result_t complete_line(line_t *line);

#endif
