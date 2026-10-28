/*
 * device_commands.c - Register the device commands with the shell.
 *
 * Two groups: the hardware itself, and how the screen looks.
 *
 * Uses: shell_begin_group / shell_register_command (shell).
 */

#include "device_commands.h"
#include "shell.h"

typedef struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} command_t;

static const command_t hardware[] = {
    { "date",    "Date and time from the real-time clock",     cmd_date    },
    { "devices", "Hardware, IRQ lines and interrupt counts",   cmd_devices },
    { "startx",  "Start the graphical desktop (ESC to leave)", cmd_startx  },
    { "beep",    "PC speaker tone: beep [hz]",                cmd_beep    },
    { "play",    "Play a short melody",                       cmd_play    },
};

static const command_t screen[] = {
    { "theme",   "Colour themes: theme [name]",                cmd_theme   },
    { "font",    "Console font: font [uzos|bios]",             cmd_font    },
    { "mode",    "Screen size: mode [80x25|80x50|1024x768]",   cmd_mode    },
};

static void add(const char *group, const command_t *list, unsigned count) {
    shell_begin_group(group);
    for (unsigned i = 0; i < count; i++)
        shell_register_command(list[i].name, list[i].description, list[i].handler);
}

void register_device_commands(void) {
    add("Devices", hardware, sizeof(hardware) / sizeof(hardware[0]));
    add("Screen", screen, sizeof(screen) / sizeof(screen[0]));
}
