#ifndef PCI_H
#define PCI_H

/*
 * pci.h - Find the devices plugged into the PCI bus.
 *
 * Every PCI device has 256 bytes of "configuration space" that say who
 * it is and where it lives. The CPU reads them through two I/O ports:
 *
 *   0xCF8  CONFIG_ADDRESS   write: enable bit | bus | device | function | offset
 *   0xCFC  CONFIG_DATA      then read or write the 32-bit value there
 *
 * The first 64 bytes (the "header") look the same on every device:
 *
 *   0x00  vendor id (16)   device id (16)      0x8086 0x100E = Intel e1000
 *   0x04  command (16)     status (16)         bit 2 of command = bus master
 *   0x08  revision, prog-if, subclass, class   class 0x02 = network
 *   0x10  BAR0 ... BAR5                        where its registers are
 *   0x3C  interrupt line (8)                   which IRQ it raises
 *
 * pci_init() asks every (bus, device, function) once at boot and keeps
 * what it found; drivers then look themselves up with pci_find().
 */

#include <stdint.h>

#define PCI_MAX_DEVICES 32

// Offsets in configuration space.
#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_CLASS_REVISION 0x08     // class << 24 | subclass << 16 | prog_if << 8 | rev
#define PCI_HEADER_TYPE    0x0E
#define PCI_BAR0           0x10
#define PCI_INTERRUPT_LINE 0x3C

// Bits of the command register.
#define PCI_CMD_IO         0x1      // the device answers its I/O ports
#define PCI_CMD_MEMORY     0x2      // the device answers its memory addresses
#define PCI_CMD_BUS_MASTER 0x4      // the device may read and write RAM itself (DMA)

#define PCI_NO_VENDOR      0xFFFF   // nobody answers at this address

// One function of one device on the bus.
typedef struct {
    uint8_t  bus, device, function;
    uint16_t vendor_id, device_id;
    uint8_t  class_code, subclass, prog_if, revision;
    uint8_t  irq;                   // interrupt line (0xFF = none)
    uint32_t bar[6];                // base address registers, as read
} pci_device_t;

// Scan the bus once. Returns how many functions were found.
int pci_init(void);

// How many functions pci_init() found, and the i-th one.
int pci_count(void);
const pci_device_t *pci_get(int index);

// The first function with this vendor and device id, or NULL.
const pci_device_t *pci_find(uint16_t vendor_id, uint16_t device_id);

// Read / write 32 bits of a function's configuration space ('offset' is 4-aligned).
uint32_t pci_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

// Turn on the command bits in 'bits' (PCI_CMD_*) of a function.
void pci_enable(const pci_device_t *dev, uint16_t bits);

#endif
