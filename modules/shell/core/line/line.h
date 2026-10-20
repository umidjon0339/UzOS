#ifndef LINE_H
#define LINE_H

/*
 * line.h - The line being typed: a fixed buffer plus its length.
 *
 * Only this file changes the buffer, and every insert asks input_bound
 * first, so the text can never run past the end of the array.
 */

#define LINE_MAX 256                    // characters, including the final NUL

typedef struct {
    char text[LINE_MAX];                // always NUL-terminated
    int  length;
    int  full;                          // already told the user it is full
} line_t;

void line_clear(line_t *line);

// Add one character at the end. Returns 1 if it fit, 0 if the line is full.
int line_insert(line_t *line, char c);

// Remove the last character. Returns 1 if there was one.
int line_backspace(line_t *line);

// Replace the whole text (cut short if too long).
void line_set(line_t *line, const char *text);

#endif
