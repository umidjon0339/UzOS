/*
 * cmd_nload.c - `nload`: the interface's speed, one line a second.
 *
 *   nload eth0 -t 5
 *   eth0  In:   12.4 KB/s ########          Out:    1.2 KB/s #
 *
 * The rate is just the difference between two readings of the
 * interface's byte counters (the ones `ip -s link` shows), one second
 * apart. The bar has one '#' per BAR_STEP bytes per second.
 *
 * Uses: netdev_by_name (core/netdev), sys_sleep_interruptible /
 *       signal_interrupted (process_management), arg_next, atoi, ui_*.
 */

#include "modules/network_management/commands/nload/cmd_nload.h"    // this command
#include "modules/network_management/core/netdev/netdev.h"          // netdev_by_name
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define BAR_STEP 1024               // bytes per second per '#'
#define BAR_MAX  16

static void speed(const char *label, uint32_t per_s) {
    ui_printf("%s", label);
    ui_color(UI_ACCENT);
    if (per_s < 1024)
        ui_printf("%6u  B/s ", per_s);
    else
        ui_printf("%4u.%u KB/s ", per_s / 1024, (per_s % 1024) * 10 / 1024);
    ui_color(UI_GOOD);
    int bars = (int)(per_s / BAR_STEP) + (per_s ? 1 : 0);
    for (int i = 0; i < BAR_MAX; i++)
        ui_printf(i < bars ? "#" : " ");
    ui_reset();
}

void cmd_nload(const char *args) {
    char word[NETDEV_NAME_LEN + 4], name[NETDEV_NAME_LEN] = "eth0";
    int seconds = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-t") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            seconds = atoi(word);
        } else {
            strcpy(name, word);
        }
    }
    netdev_t *dev = netdev_by_name(name);
    if (!dev) {
        ui_usage("nload [dev] [-t seconds]");
        return;
    }
    ui_printf("nload: %s, once a second, Ctrl+C to stop\n", dev->name);
    uint32_t rx = dev->stats.rx_bytes, tx = dev->stats.tx_bytes;
    for (int t = 0; !signal_interrupted() && (!seconds || t < seconds); t++) {
        for (int w = 0; w < 1000 && !signal_interrupted(); w += 50)
            sys_sleep_interruptible(50);
        uint32_t now_rx = dev->stats.rx_bytes, now_tx = dev->stats.tx_bytes;
        ui_printf("%-5s ", dev->name);
        speed("In: ", now_rx - rx);
        speed("  Out: ", now_tx - tx);
        ui_printf("\n");
        rx = now_rx;
        tx = now_tx;
    }
}
