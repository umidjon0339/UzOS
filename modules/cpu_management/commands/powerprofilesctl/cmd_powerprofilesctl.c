/*
 * cmd_powerprofilesctl.c - `powerprofilesctl`: fast, balanced or frugal?
 *
 *   powerprofilesctl list           the three profiles, '*' = the active one
 *   powerprofilesctl get            the active profile's name
 *   powerprofilesctl set NAME       switch (performance, balanced, power-saver)
 *
 * What each profile really changes in UzOS is in core/power: how the
 * idle CPU waits (spin or hlt) and how long one turn on the CPU lasts.
 * Like on Linux, any logged-in user may switch.
 *
 * Uses: power_* (core/power), ui_* (ui), arg_next / strcmp (lib).
 */

#include "modules/cpu_management/commands/powerprofilesctl/cmd_powerprofilesctl.h"
#include "modules/cpu_management/core/power/power.h"        // power_*
#include "modules/ui/core/style/ui.h"                       // ui_*
#include "lib/args/args.h"                                  // arg_next
#include "lib/string/string.h"                              // strcmp

static const char *what_it_does[POWER_PROFILE_COUNT] = {
    "idle: spin (POLL), turns of 50 ms",
    "idle: hlt (C1), turns of 50 ms",
    "idle: hlt (C1), turns of 100 ms (fewer task switches)",
};

static void list(void) {
    for (int p = POWER_PROFILE_COUNT - 1; p >= 0; p--) {
        int active = power_profile() == (power_profile_t)p;
        ui_color(active ? UI_GOOD : ui_text_fg());
        ui_printf("%s %s:\n", active ? "*" : " ", power_profile_name(p));
        ui_color(UI_DIM);
        ui_printf("    Driver:     uzos-power\n    Effect:     %s\n\n", what_it_does[p]);
        ui_reset();
    }
}

void cmd_powerprofilesctl(const char *args) {
    char verb[16], name[24];
    const char *rest = arg_next(args, verb, sizeof(verb));
    arg_next(rest, name, sizeof(name));

    if (!verb[0] || strcmp(verb, "list") == 0) {
        list();
    } else if (strcmp(verb, "get") == 0) {
        ui_printf("%s\n", power_profile_name(power_profile()));
    } else if (strcmp(verb, "set") == 0 && name[0]) {
        int p = power_profile_find(name);
        if (p < 0) {
            ui_error("powerprofilesctl", "no such profile (performance, balanced, power-saver)");
            return;
        }
        power_set_profile(p);
    } else {
        ui_usage("powerprofilesctl [list | get | set performance|balanced|power-saver]");
    }
}
