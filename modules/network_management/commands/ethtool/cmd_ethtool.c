/*
 * cmd_ethtool.c - `ethtool [-i | -S | -d | -t] [eth0]`: look at the card.
 *
 *   ethtool       link: speed, duplex, is a cable in
 *   ethtool -i    driver, PCI address, IRQ, MAC
 *   ethtool -S    counters: the driver's and the card's own
 *   ethtool -d    a register dump: CTRL, STATUS, the ring heads and tails
 *   ethtool -t    self-test: registers, EEPROM, interrupt, loopback, link
 *
 * The loopback test is the card's own "talk to yourself": with bit 14
 * of the PHY's control register set, a frame we send comes straight
 * back in instead of going onto the cable. We borrow the receiver for a
 * moment to catch it.
 *
 * Uses: e1000_* (core/e1000), sys_sleep (process_management),
 *       arg_next (lib/args), strcmp (lib/string), ui_* (ui).
 */

#include "modules/network_management/commands/ethtool/cmd_ethtool.h"   // this command
#include "modules/network_management/core/e1000/e1000.h"               // e1000_*
#include "modules/process_management/syscalls/sleep/sys_sleep.h"       // sys_sleep
#include "lib/args/args.h"                                             // arg_next
#include "lib/string/string.h"                                         // strcmp, memcpy, memset
#include "modules/ui/core/style/ui.h"                                  // ui_*

#define EEPROM_WORDS     0x40       // the checksum covers words 0x00-0x3F
#define EEPROM_SUM       0xBABA     // ...and they must add up to this
#define PHY_CONTROL      0          // PHY register 0 (BMCR)
#define PHY_LOOPBACK     0x4000     // bit 14: send back to ourselves
#define TEST_ETHERTYPE   0x88B5     // "local experimental": nobody else uses it
#define TEST_LEN         64
#define INT_LSC          (1u << 2)  // the interrupt cause the test raises

/* ---------- ethtool ---------- */

static void show_settings(void) {
    ui_printf("Settings for eth0:\n");
    ui_printf("    Supported ports:      [ TP ]\n");
    ui_printf("    Supported link modes: 10baseT/Full 100baseT/Full 1000baseT/Full\n");
    ui_printf("    Speed:                %dMb/s\n", e1000_speed());
    ui_printf("    Duplex:               %s\n", e1000_full_duplex() ? "Full" : "Half");
    ui_printf("    Port:                 Twisted Pair\n");
    ui_printf("    Auto-negotiation:     on\n");
    ui_printf("    Promiscuous:          %s\n", e1000_promiscuous() ? "on" : "off");
    ui_printf("    Link detected:        ");
    ui_color(e1000_link_up() ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", e1000_link_up() ? "yes" : "no");
    ui_reset();
}

/* ---------- ethtool -i ---------- */

static void show_driver(void) {
    uint8_t mac[E1000_MAC_LEN];
    e1000_mac(mac);
    ui_printf("driver:   e1000 (UzOS, network_management/core/e1000)\n");
    ui_printf("bus-info: 0000:%s\n", e1000_bus_info());
    ui_printf("irq:      %u\n", e1000_irq());
    ui_printf("mac:      %02x:%02x:%02x:%02x:%02x:%02x\n",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* ---------- ethtool -S ---------- */

static void show_stats(void) {
    const e1000_stats_t *s = e1000_stats();
    ui_printf("NIC statistics (driver):\n");
    ui_printf("    rx_packets:   %u\n", s->rx_packets);
    ui_printf("    rx_bytes:     %u\n", s->rx_bytes);
    ui_printf("    rx_errors:    %u\n", s->rx_errors);
    ui_printf("    rx_dropped:   %u\n", s->rx_dropped);
    ui_printf("    tx_packets:   %u\n", s->tx_packets);
    ui_printf("    tx_bytes:     %u\n", s->tx_bytes);
    ui_printf("    tx_errors:    %u\n", s->tx_errors);
    ui_printf("    interrupts:   %u\n", s->interrupts);
    ui_printf("    link_changes: %u\n", s->link_changes);
    // The card's counters reset when read: these are "since last time".
    ui_printf("NIC statistics (card, since the last ethtool -S):\n");
    ui_printf("    good_rx:      %u\n", e1000_reg_read(E1000_GPRC));
    ui_printf("    good_tx:      %u\n", e1000_reg_read(E1000_GPTC));
    ui_printf("    crc_errors:   %u\n", e1000_reg_read(E1000_CRCERRS));
    ui_printf("    missed:       %u\n", e1000_reg_read(E1000_MPC));
}

/* ---------- ethtool -d ---------- */

static void show_registers(void) {
    static const struct { uint32_t offset; const char *name, *meaning; } regs[] = {
        { E1000_CTRL,   "CTRL",   "device control" },
        { E1000_STATUS, "STATUS", "link up, speed, duplex" },
        { E1000_IMS,    "IMS",    "interrupt causes enabled" },
        { E1000_RCTL,   "RCTL",   "receive control" },
        { E1000_TCTL,   "TCTL",   "transmit control" },
        { E1000_RDBAL,  "RDBAL",  "RX ring address" },
        { E1000_RDLEN,  "RDLEN",  "RX ring bytes" },
        { E1000_RDH,    "RDH",    "RX head (card)" },
        { E1000_RDT,    "RDT",    "RX tail (driver)" },
        { E1000_TDBAL,  "TDBAL",  "TX ring address" },
        { E1000_TDLEN,  "TDLEN",  "TX ring bytes" },
        { E1000_TDH,    "TDH",    "TX head (card)" },
        { E1000_TDT,    "TDT",    "TX tail (driver)" },
        { E1000_RAL,    "RAL",    "our MAC, low" },
        { E1000_RAH,    "RAH",    "our MAC, high + valid" },
    };
    ui_color(UI_TITLE);
    ui_printf("offset  register  value     meaning\n");
    ui_reset();
    for (unsigned i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
        ui_printf("0x%04x  %-8s  %08x  %s\n", regs[i].offset, regs[i].name,
                  e1000_reg_read(regs[i].offset), regs[i].meaning);
}

/* ---------- ethtool -t ---------- */

static volatile int loop_got;               // the test frame came back
static uint8_t loop_frame[TEST_LEN];

static void catch_frame(const uint8_t *frame, uint16_t len) {
    if (len < TEST_LEN)
        return;
    for (int i = 0; i < TEST_LEN; i++)
        if (frame[i] != loop_frame[i])
            return;
    loop_got = 1;
}

// Write two patterns to a spare register and read them back.
static int test_registers(void) {
    uint32_t spare = E1000_MTA + 127 * 4;   // the last multicast word: unused
    int ok = 1;
    e1000_reg_write(spare, 0x5A5A5A5A);
    ok &= e1000_reg_read(spare) == 0x5A5A5A5A;
    e1000_reg_write(spare, 0xA5A5A5A5);
    ok &= e1000_reg_read(spare) == 0xA5A5A5A5;
    e1000_reg_write(spare, 0);
    return ok;
}

static int test_eeprom(void) {
    uint16_t sum = 0;
    for (int i = 0; i < EEPROM_WORDS; i++) {
        int word = e1000_eeprom_read((uint8_t)i);
        if (word < 0)
            return 0;
        sum += (uint16_t)word;
    }
    return sum == EEPROM_SUM;
}

// Raise an interrupt by hand (ICS) and see the driver count it.
static int test_interrupt(void) {
    uint32_t before = e1000_stats()->interrupts;
    e1000_reg_write(E1000_ICS, INT_LSC);
    sys_sleep(20);
    return e1000_stats()->interrupts > before;
}

static int test_loopback(void) {
    // A frame to ourselves: dest = src = our MAC, then a counting pattern.
    e1000_mac(loop_frame);
    e1000_mac(loop_frame + 6);
    loop_frame[12] = TEST_ETHERTYPE >> 8;
    loop_frame[13] = TEST_ETHERTYPE & 0xFF;
    for (int i = 14; i < TEST_LEN; i++)
        loop_frame[i] = (uint8_t)i;

    int control = e1000_phy_read(PHY_CONTROL);
    if (control < 0)
        return 0;
    e1000_receiver_t old = e1000_set_receiver(catch_frame);
    loop_got = 0;
    e1000_phy_write(PHY_CONTROL, (uint16_t)(control | PHY_LOOPBACK));
    e1000_send(loop_frame, TEST_LEN);
    for (int i = 0; i < 10 && !loop_got; i++)
        sys_sleep(10);
    e1000_phy_write(PHY_CONTROL, (uint16_t)control);
    e1000_set_receiver(old);
    return loop_got;
}

static void result(const char *name, int ok) {
    ui_printf("    %-22s", name);
    ui_color(ok ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", ok ? "PASS" : "FAIL");
    ui_reset();
}

static void self_test(void) {
    int ok[5];
    ok[0] = test_registers();
    ok[1] = test_eeprom();
    ok[2] = test_interrupt();
    ok[3] = test_loopback();
    ok[4] = e1000_link_up();
    int all = ok[0] && ok[1] && ok[2] && ok[3] && ok[4];
    ui_printf("The test result is ");
    ui_color(all ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", all ? "PASS" : "FAIL");
    ui_reset();
    ui_printf("The test extra info:\n");
    result("Register test", ok[0]);
    result("EEPROM checksum", ok[1]);
    result("Interrupt test", ok[2]);
    result("Loopback test", ok[3]);
    result("Link test", ok[4]);
}

void cmd_ethtool(const char *args) {
    char word[8];
    char option = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] == '-' && word[1] && !word[2] && !option) {
            option = word[1];
        } else if (strcmp(word, "eth0") != 0) {
            ui_usage("ethtool [-i | -S | -d | -t] [eth0]");
            return;
        }
    }
    if (!e1000_present()) {
        ui_error("ethtool", "no e1000 network card (QEMU: -nic user,model=e1000)");
        return;
    }
    switch (option) {
    case 0:   show_settings();  break;
    case 'i': show_driver();    break;
    case 'S': show_stats();     break;
    case 'd': show_registers(); break;
    case 't': self_test();      break;
    default:  ui_usage("ethtool [-i | -S | -d | -t] [eth0]");
    }
}
