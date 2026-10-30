/*
 * cmd_lspci.c - `lspci [-n] [-v]`: what is plugged into the PCI bus.
 *
 *   lspci       00:03.0 Ethernet controller: Intel 82540EM Gigabit Ethernet (rev 03)
 *   lspci -n    00:03.0 0200: 8086:100e (rev 03)          numbers only
 *   lspci -v    ...and each device's IRQ and BARs (where its registers are)
 *
 * The bus was scanned once at boot (core/pci); this only prints it. The
 * names below are a small table of the devices QEMU and VirtualBox show;
 * anything else prints as its numbers.
 *
 * Uses: pci_count / pci_get (core/pci), arg_next (lib/args), strcmp
 *       (lib/string), ui_* (ui).
 */

#include "modules/network_management/commands/lspci/cmd_lspci.h"   // this command
#include "modules/network_management/core/pci/pci.h"                // pci_count, pci_get
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcmp
#include "modules/ui/core/style/ui.h"                               // ui_printf, ui_color, ui_usage

/* ---------- Names ---------- */

static const struct { uint16_t id; const char *name; } vendors[] = {
    { 0x8086, "Intel" },     { 0x1234, "QEMU" },       { 0x1af4, "Red Hat (virtio)" },
    { 0x80ee, "VirtualBox" }, { 0x10ec, "Realtek" },   { 0x1022, "AMD" },
    { 0x15ad, "VMware" },    { 0x106b, "Apple" },
};

static const struct { uint16_t vendor, device; const char *name; } models[] = {
    { 0x8086, 0x1237, "440FX chipset" },
    { 0x8086, 0x7000, "PIIX3 ISA bridge" },
    { 0x8086, 0x7010, "PIIX3 IDE" },
    { 0x8086, 0x7020, "PIIX3 USB" },
    { 0x8086, 0x7111, "PIIX4 IDE" },
    { 0x8086, 0x7113, "PIIX4 ACPI" },
    { 0x8086, 0x100e, "82540EM Gigabit Ethernet" },     // QEMU and VirtualBox default: e1000
    { 0x8086, 0x100f, "82545EM Gigabit Ethernet" },
    { 0x8086, 0x10d3, "82574L Gigabit Ethernet" },
    { 0x8086, 0x2415, "82801AA AC'97 Audio" },
    { 0x10ec, 0x8139, "RTL-8139 Fast Ethernet" },
    { 0x1234, 0x1111, "Standard VGA (BGA)" },
    { 0x80ee, 0xbeef, "Graphics Adapter" },
    { 0x80ee, 0xcafe, "Guest Service" },
    { 0x106b, 0x003f, "KeyLargo USB" },
};

static const struct { uint8_t class_code, subclass; const char *name; } classes[] = {
    { 0x01, 0x01, "IDE interface" },        { 0x01, 0x06, "SATA controller" },
    { 0x02, 0x00, "Ethernet controller" },  { 0x03, 0x00, "VGA compatible controller" },
    { 0x04, 0x01, "Multimedia audio controller" },
    { 0x06, 0x00, "Host bridge" },          { 0x06, 0x01, "ISA bridge" },
    { 0x06, 0x04, "PCI bridge" },           { 0x06, 0x80, "Bridge" },
    { 0x08, 0x80, "System peripheral" },    { 0x0c, 0x03, "USB controller" },
};

static const char *vendor_name(uint16_t id) {
    for (unsigned i = 0; i < sizeof(vendors) / sizeof(vendors[0]); i++)
        if (vendors[i].id == id)
            return vendors[i].name;
    return 0;
}

static const char *model_name(uint16_t vendor, uint16_t device) {
    for (unsigned i = 0; i < sizeof(models) / sizeof(models[0]); i++)
        if (models[i].vendor == vendor && models[i].device == device)
            return models[i].name;
    return 0;
}

static const char *class_name(uint8_t class_code, uint8_t subclass) {
    for (unsigned i = 0; i < sizeof(classes) / sizeof(classes[0]); i++)
        if (classes[i].class_code == class_code && classes[i].subclass == subclass)
            return classes[i].name;
    return 0;
}

/* ---------- Printing ---------- */

// "  IRQ 11, BAR0 memory at febc0000, BAR1 I/O ports at c000"
static void print_details(const pci_device_t *d) {
    ui_color(UI_DIM);
    if (d->irq && d->irq != 0xFF)
        ui_printf("        IRQ %u\n", d->irq);
    for (int i = 0; i < 6; i++) {
        uint32_t bar = d->bar[i];
        if (!bar)
            continue;
        // Bit 0 says which address space: 1 = I/O ports, 0 = memory.
        if (bar & 1)
            ui_printf("        BAR%d  I/O ports at %x\n", i, bar & ~3u);
        else
            ui_printf("        BAR%d  memory at %08x%s\n", i, bar & ~0xFu,
                      (bar & 8) ? " (prefetchable)" : "");
        if ((bar & 7) == 4)         // a 64-bit BAR uses the next one too
            i++;
    }
    ui_reset();
}

static void print_device(const pci_device_t *d, int numeric, int verbose) {
    ui_color(UI_LINK);
    ui_printf("%02x:%02x.%u ", d->bus, d->device, d->function);
    ui_reset();

    const char *cls = class_name(d->class_code, d->subclass);
    const char *vendor = vendor_name(d->vendor_id);
    const char *model = model_name(d->vendor_id, d->device_id);

    if (numeric || !cls)
        ui_printf("%02x%02x: ", d->class_code, d->subclass);
    else
        ui_printf("%s: ", cls);

    if (numeric || !vendor)
        ui_printf("%04x:%04x", d->vendor_id, d->device_id);
    else if (model)
        ui_printf("%s %s", vendor, model);
    else
        ui_printf("%s device %04x", vendor, d->device_id);

    ui_printf(" (rev %02x)\n", d->revision);
    if (verbose)
        print_details(d);
}

void cmd_lspci(const char *args) {
    int numeric = 0, verbose = 0;
    char word[8];
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-n") == 0) {
            numeric = 1;
        } else if (strcmp(word, "-v") == 0) {
            verbose = 1;
        } else {
            ui_usage("lspci [-n] [-v]");
            return;
        }
    }

    if (pci_count() == 0) {
        ui_printf("no PCI devices found\n");
        return;
    }
    for (int i = 0; i < pci_count(); i++)
        print_device(pci_get(i), numeric, verbose);
}
