/*
 * highlight.c - Find the command word and pick its colour.
 *
 * Uses: shell_command_count / shell_command (core/shell) for the list of
 *       commands, strncmp / strlen (lib/string), ui_* colours (ui module),
 *       vga_put_char (device_management).
 */

#include "highlight.h"
#include "shell.h"
#include "string.h"
#include "ui.h"

enum vga_color highlight_command_color(const char *word, int len) {
    int starts_one = 0;

    for (int i = 0; i < shell_command_count(); i++) {
        const char *name = shell_command(i)->name;
        if (strncmp(name, word, len) != 0)
            continue;
        if (name[len] == '\0')
            return UI_GOOD;             // exactly a command
        starts_one = 1;                 // the start of one
    }
    return starts_one ? ui_text_fg() : UI_BAD;
}

void highlight_print(const char *text) {
    int start = 0;
    while (text[start] == ' ')
        start++;                        // spaces before the command
    int end = start;
    while (text[end] && text[end] != ' ')
        end++;                          // the command word: text[start..end)

    for (int i = 0; text[i]; i++) {
        if (i == start && end > start)
            ui_color(highlight_command_color(text + start, end - start));
        if (i == end)
            ui_reset();                 // arguments: the normal colour
        vga_put_char(text[i]);
    }
    ui_reset();
}
