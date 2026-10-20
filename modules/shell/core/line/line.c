/*
 * line.c - Edit the input line safely.
 *
 * Uses: the defence input_bound before every insert.
 */

#include "line.h"
#include "def_input_bound.h"

void line_clear(line_t *line) {
    line->text[0] = '\0';
    line->length = 0;
    line->full = 0;
}

int line_insert(line_t *line, char c) {
    // DEFENCE input_bound: never write past the last slot (kept for NUL).
    // Report a full line once, not for every extra key.
    if (line->full || !def_input_room_ok(line->length, LINE_MAX)) {
        line->full = 1;
        return 0;
    }
    line->text[line->length++] = c;
    line->text[line->length] = '\0';
    return 1;
}

int line_backspace(line_t *line) {
    if (line->length == 0)
        return 0;
    line->text[--line->length] = '\0';
    line->full = 0;
    return 1;
}

void line_set(line_t *line, const char *text) {
    line_clear(line);
    while (*text && line->length < LINE_MAX - 1)
        line->text[line->length++] = *text++;
    line->text[line->length] = '\0';
}
