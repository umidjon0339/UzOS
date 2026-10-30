/*
 * pci.c - Scan the PCI bus through the configuration ports 0xCF8 / 0xCFC.
 *
 * A PCI address is (bus 0-255, device 0-31, function 0-7). Asking an
 * empty address reads vendor 0xFFFF. A device with bit 7 of its header
 * type set has more than one function (QEMU's chipset at 0:1 does).
 *
 * Uses: outl / inl (arch/x86/io).
 */

#include "modules/network_management/core/pci/pci.h"     // this file
#include "arch/x86/io/io.h"                               // outl, inl

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA    0xCFC
#define ENABLE_BIT     0x80000000u      // bit 31: "this is a configuration access"
#define MULTI_FUNCTION 0x80             // header type bit 7

static pci_device_t devices[PCI_MAX_DEVICES];
static int device_count;

/* ---------- Configuration space ---------- */

// The value for CONFIG_ADDRESS:  1 | bus(8) | device(5) | function(3) | offset(8)
static uint32_t config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return ENABLE_BIT | ((uint32_t)bus << 16) | ((uint32_t)(device & 31) << 11) |
           ((uint32_t)(function & 7) << 8) | (offset & 0xFC);
}

uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    outl(CONFIG_ADDRESS, config_address(bus, device, function, offset));
    return inl(CONFIG_DATA);
}

void pci_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    outl(CONFIG_ADDRESS, config_address(bus, device, function, offset));
    outl(CONFIG_DATA, value);
}

// 16 or 8 bits are a part of the 32-bit value around them.
static uint16_t read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return (uint16_t)(pci_read32(bus, device, function, offset) >> ((offset & 2) * 8));
}

static uint8_t read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return (uint8_t)(pci_read32(bus, device, function, offset) >> ((offset & 3) * 8));
}

/* ---------- Scanning ---------- */

// Remember the function at (bus, device, function).
static void add(uint8_t bus, uint8_t device, uint8_t function) {
    if (device_count >= PCI_MAX_DEVICES)
        return;
    pci_device_t *d = &devices[device_count++];
    uint32_t class_rev = pci_read32(bus, device, function, PCI_CLASS_REVISION);

    d->bus = bus;
    d->device = device;
    d->function = function;
    d->vendor_id  = read16(bus, device, function, PCI_VENDOR_ID);
    d->device_id  = read16(bus, device, function, PCI_DEVICE_ID);
    d->class_code = (uint8_t)(class_rev >> 24);
    d->subclass   = (uint8_t)(class_rev >> 16);
    d->prog_if    = (uint8_t)(class_rev >> 8);
    d->revision   = (uint8_t)class_rev;
    d->irq        = read8(bus, device, function, PCI_INTERRUPT_LINE);
    for (int i = 0; i < 6; i++)
        d->bar[i] = pci_read32(bus, device, function, (uint8_t)(PCI_BAR0 + i * 4));
}

int pci_init(void) {
    device_count = 0;
    // Brute force: ask every address. 256 x 32 reads is quick.
    for (int bus = 0; bus < 256; bus++) {
        for (int dev = 0; dev < 32; dev++) {
            if (read16(bus, dev, 0, PCI_VENDOR_ID) == PCI_NO_VENDOR)
                continue;
            add(bus, dev, 0);
            if (!(read8(bus, dev, 0, PCI_HEADER_TYPE) & MULTI_FUNCTION))
                continue;
            for (int fn = 1; fn < 8; fn++)
                if (read16(bus, dev, fn, PCI_VENDOR_ID) != PCI_NO_VENDOR)
                    add(bus, dev, fn);
        }
    }
    return device_count;
}

/* ---------- Lookups ---------- */

int pci_count(void) {
    return device_count;
}

const pci_device_t *pci_get(int index) {
    return index >= 0 && index < device_count ? &devices[index] : 0;
}

const pci_device_t *pci_find(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < device_count; i++)
        if (devices[i].vendor_id == vendor_id && devices[i].device_id == device_id)
            return &devices[i];
    return 0;
}

void pci_enable(const pci_device_t *dev, uint16_t bits) {
    // The command register is the low half of offset 0x04; the high
    // half (status) clears a bit when 1 is written, so write 0 there.
    uint32_t value = pci_read32(dev->bus, dev->device, dev->function, PCI_COMMAND) & 0xFFFF;
    pci_write32(dev->bus, dev->device, dev->function, PCI_COMMAND, value | bits);
}
