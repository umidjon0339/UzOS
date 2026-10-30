/*
 * e1000.c - Intel e1000 (82540EM) driver: start the card, send, receive.
 *
 * Start-up, in the order Intel's manual asks for:
 *   1. PCI: let the card answer its memory addresses and use DMA
 *   2. map its registers (BAR0) into the page tables, uncached
 *   3. reset it, then "set link up"
 *   4. read our MAC from the EEPROM, and tell the receive filter
 *   5. build the RX and TX rings, turn receive and transmit on
 *   6. hook its IRQ and unmask the interrupt causes we want
 *
 * DMA needs PHYSICAL addresses. The rings and buffers below are static
 * arrays inside the kernel, which lives in the identity-mapped first
 * 8 MiB (memory_layout.h): there an address IS its physical address,
 * so the driver can hand them to the card as they are.
 *
 * Receive runs inside the card's interrupt: each frame goes straight to
 * the receiver (netdev, which passes it up the stack).
 *
 * Uses: pci_find / pci_enable (core/pci), vmm_map_page / vmm_walk
 *       (memory_management), register_interrupt_handler (arch/x86/isr),
 *       pic_unmask (device_management), cpu_irq_save (arch/x86/cpu),
 *       memcpy (lib/string), format (lib/format).
 */

#include "modules/network_management/core/e1000/e1000.h"         // this file
#include "modules/network_management/core/pci/pci.h"             // pci_find, pci_enable
#include "modules/memory_management/core/vmm/vmm.h"              // vmm_map_page, vmm_walk, PAGE_*
#include "modules/memory_management/core/layout/memory_layout.h" // IDENTITY_MAP_END, PAGE_SIZE
#include "modules/device_management/pic/pic.h"                   // pic_unmask
#include "arch/x86/isr/isr.h"                                    // register_interrupt_handler, IRQ_BASE
#include "arch/x86/cpu/cpu.h"                                    // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                   // memcpy, memset
#include "lib/format/format.h"                                   // format
#include "modules/network_management/security/rx_ring_overflow/defence/rx_desc_check/def_rx_desc_check.h"  // def_rx_desc_ok

#define INTEL_VENDOR  0x8086
#define MMIO_SIZE     0x20000           // 128 KiB of registers

#define RX_COUNT      32                // receive descriptors (RDLEN: a multiple of 8)
#define TX_COUNT      8                 // transmit descriptors
#define BUFFER_SIZE   2048              // one frame per buffer (RCTL.BSIZE = 2048)

// CTRL bits
#define CTRL_ASDE     (1u << 5)         // find the link speed by itself
#define CTRL_SLU      (1u << 6)         // set link up
#define CTRL_RST      (1u << 26)        // reset the whole card
#define CTRL_LRST     (1u << 3)         // link reset
#define CTRL_PHY_RST  (1u << 31)        // PHY reset

// STATUS bits
#define STATUS_FD     (1u << 0)         // full duplex
#define STATUS_LU     (1u << 1)         // link up
#define STATUS_SPEED_SHIFT 6            // bits 6-7: 0 = 10, 1 = 100, 2/3 = 1000 Mb/s

// RCTL bits
#define RCTL_EN       (1u << 1)         // receive on
#define RCTL_UPE      (1u << 3)         // unicast promiscuous
#define RCTL_MPE      (1u << 4)         // multicast promiscuous
#define RCTL_BAM      (1u << 15)        // accept broadcast (ff:ff:ff:ff:ff:ff)
#define RCTL_SECRC    (1u << 26)        // strip the CRC before giving us the frame
                                        // BSIZE bits 16-17 = 0: 2048-byte buffers

// TCTL bits
#define TCTL_EN       (1u << 1)         // transmit on
#define TCTL_PSP      (1u << 3)         // pad short frames to 64 bytes
#define TCTL_CT       (0x10u << 4)      // collision retries (half duplex only)
#define TCTL_COLD     (0x40u << 12)     // collision distance
#define TIPG_DEFAULT  (10u | (8u << 10) | (6u << 20))   // the manual's gap for copper

// Interrupt causes (ICR / IMS)
#define INT_LSC       (1u << 2)         // link status changed
#define INT_RXDMT0    (1u << 4)         // RX ring almost empty
#define INT_RXO       (1u << 6)         // RX overrun: a frame was lost
#define INT_RXT0      (1u << 7)         // a frame arrived

// EEPROM read (EERD)
#define EERD_START    1u
#define EERD_DONE     (1u << 4)

// PHY access (MDIC)
#define MDIC_PHY1     (1u << 21)        // PHY address 1
#define MDIC_WRITE    (1u << 26)
#define MDIC_READ     (2u << 26)
#define MDIC_READY    (1u << 28)
#define MDIC_ERROR    (1u << 30)

#define RAH_VALID     (1u << 31)        // the receive address is in use

// Descriptor bits
#define DESC_DD       (1u << 0)         // status: descriptor done
#define DESC_EOP      (1u << 1)         // RX status: end of packet
#define TXCMD_EOP     (1u << 0)         // TX command: last buffer of the frame
#define TXCMD_IFCS    (1u << 1)         //             insert the CRC
#define TXCMD_RS      (1u << 3)         //             report status (set DD)

// A receive descriptor, exactly as the card reads it (16 bytes).
typedef struct {
    uint64_t address;                   // physical address of the buffer
    uint16_t length;                    // bytes the card wrote
    uint16_t checksum;
    uint8_t  status;                    // DESC_DD, DESC_EOP
    uint8_t  errors;
    uint16_t special;
} __attribute__((packed)) rx_desc_t;

// A transmit descriptor ("legacy" format, 16 bytes).
typedef struct {
    uint64_t address;
    uint16_t length;
    uint8_t  cso;                       // checksum offset (unused)
    uint8_t  cmd;                       // TXCMD_*
    uint8_t  status;                    // DESC_DD when sent
    uint8_t  css;
    uint16_t special;
} __attribute__((packed)) tx_desc_t;

// The rings must start on a 16-byte boundary; 128 keeps them in one cache line group.
static rx_desc_t rx_ring[RX_COUNT] __attribute__((aligned(128)));
static tx_desc_t tx_ring[TX_COUNT] __attribute__((aligned(128)));
static uint8_t   rx_buffers[RX_COUNT][BUFFER_SIZE] __attribute__((aligned(16)));
static uint8_t   tx_buffers[TX_COUNT][BUFFER_SIZE] __attribute__((aligned(16)));

static volatile uint8_t *mmio;          // NULL until a card is found
static const pci_device_t *pci;
static uint8_t mac[E1000_MAC_LEN];
static uint32_t rx_next;                // next RX descriptor to look at
static uint32_t tx_tail;                // next TX descriptor to fill
static e1000_receiver_t receiver;
static e1000_stats_t stats;
static char bus_info[12];

/* ---------- Registers ---------- */

uint32_t e1000_reg_read(uint32_t offset) {
    return *(volatile uint32_t *)(mmio + offset);
}

void e1000_reg_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)(mmio + offset) = value;
}

int e1000_eeprom_read(uint8_t word) {
    e1000_reg_write(E1000_EERD, ((uint32_t)word << 8) | EERD_START);
    for (int i = 0; i < 100000; i++) {
        uint32_t value = e1000_reg_read(E1000_EERD);
        if (value & EERD_DONE)
            return (int)(value >> 16);
    }
    return -1;
}

// Wait for an MDIC operation; returns its data, or -1.
static int mdic_wait(void) {
    for (int i = 0; i < 100000; i++) {
        uint32_t value = e1000_reg_read(E1000_MDIC);
        if (value & MDIC_ERROR)
            return -1;
        if (value & MDIC_READY)
            return (int)(value & 0xFFFF);
    }
    return -1;
}

int e1000_phy_read(uint8_t reg) {
    e1000_reg_write(E1000_MDIC, MDIC_READ | MDIC_PHY1 | ((uint32_t)reg << 16));
    return mdic_wait();
}

int e1000_phy_write(uint8_t reg, uint16_t value) {
    e1000_reg_write(E1000_MDIC, MDIC_WRITE | MDIC_PHY1 | ((uint32_t)reg << 16) | value);
    return mdic_wait() < 0 ? -1 : 0;
}

/* ---------- Receiving (in the interrupt) ---------- */

// Take every frame the card has finished, oldest first.
static void receive_all(void) {
    while (rx_ring[rx_next].status & DESC_DD) {
        rx_desc_t *d = &rx_ring[rx_next];
        uint16_t len = d->length;

        // DEFENCE rx_desc_check: never trust the length the card wrote.
        if (!def_rx_desc_ok(d->status, d->errors, len, BUFFER_SIZE)) {
            stats.rx_errors++;              // bad CRC, a frame split over buffers, a lying length
        } else if (!receiver) {
            stats.rx_dropped++;             // nobody listens yet
        } else {
            stats.rx_packets++;
            stats.rx_bytes += len;
            receiver(rx_buffers[rx_next], len);
        }

        // Give the buffer back: clear it, then move the tail onto it.
        d->status = 0;
        e1000_reg_write(E1000_RDT, rx_next);
        rx_next = (rx_next + 1) % RX_COUNT;
    }
}

static void e1000_interrupt(registers_t *regs) {
    (void)regs;
    uint32_t cause = e1000_reg_read(E1000_ICR);     // reading also clears it
    stats.interrupts++;

    if (cause & INT_LSC)
        stats.link_changes++;
    if (cause & INT_RXO)
        stats.rx_dropped++;                         // the ring was full
    if (cause & (INT_RXT0 | INT_RXDMT0 | INT_RXO))
        receive_all();
}

/* ---------- Sending ---------- */

int e1000_send(const void *frame, uint16_t len) {
    if (!mmio || len == 0 || len > E1000_MAX_FRAME - 4)
        return 0;

    uint32_t flags = cpu_irq_save();            // the stack may send from an interrupt too
    tx_desc_t *d = &tx_ring[tx_tail];
    if (!(d->status & DESC_DD)) {               // still waiting to go out: ring full
        stats.tx_errors++;
        cpu_irq_restore(flags);
        return 0;
    }
    memcpy(tx_buffers[tx_tail], frame, len);
    d->length = len;
    d->cmd = TXCMD_EOP | TXCMD_IFCS | TXCMD_RS;
    d->status = 0;
    tx_tail = (tx_tail + 1) % TX_COUNT;
    e1000_reg_write(E1000_TDT, tx_tail);        // "go": the card sends up to the tail

    stats.tx_packets++;
    stats.tx_bytes += len;
    cpu_irq_restore(flags);
    return 1;
}

/* ---------- Start-up ---------- */

// Identity-map the register area, uncached. Refuses if it would hide
// something already mapped there.
static int map_registers(uint32_t base) {
    for (uint32_t off = 0; off < MMIO_SIZE; off += PAGE_SIZE) {
        vmm_walk_t walk;
        if (vmm_walk(base + off, &walk) && walk.physical != base + off)
            return 0;
        if (!vmm_map_page(base + off, base + off, PAGE_WRITABLE | PAGE_NO_CACHE))
            return 0;
    }
    mmio = (volatile uint8_t *)base;
    return 1;
}

static void reset(void) {
    e1000_reg_write(E1000_IMC, 0xFFFFFFFF);     // no interrupts while we work
    e1000_reg_write(E1000_CTRL, e1000_reg_read(E1000_CTRL) | CTRL_RST);
    for (int i = 0; i < 100000 && (e1000_reg_read(E1000_CTRL) & CTRL_RST); i++)
        ;                                       // the bit clears when the reset is done
    e1000_reg_write(E1000_IMC, 0xFFFFFFFF);     // the reset turned the mask back on
    e1000_reg_read(E1000_ICR);

    uint32_t ctrl = e1000_reg_read(E1000_CTRL);
    ctrl &= ~(CTRL_LRST | CTRL_PHY_RST);
    e1000_reg_write(E1000_CTRL, ctrl | CTRL_SLU | CTRL_ASDE);
}

// The receive filter: accept frames sent to this address.
static void program_filter(void) {
    e1000_reg_write(E1000_RAL, mac[0] | (mac[1] << 8) | (mac[2] << 16) | ((uint32_t)mac[3] << 24));
    e1000_reg_write(E1000_RAH, mac[4] | (mac[5] << 8) | RAH_VALID);
}

static void read_mac(void) {
    for (int i = 0; i < 3; i++) {
        int word = e1000_eeprom_read((uint8_t)i);     // words 0-2: the MAC, low byte first
        mac[i * 2]     = (uint8_t)word;
        mac[i * 2 + 1] = (uint8_t)(word >> 8);
    }
    program_filter();
    for (int i = 0; i < 128; i++)
        e1000_reg_write(E1000_MTA + i * 4, 0);    // no multicast groups
}

static void setup_rx(void) {
    for (int i = 0; i < RX_COUNT; i++) {
        rx_ring[i].address = (uint32_t)rx_buffers[i];   // identity mapped: physical
        rx_ring[i].status = 0;
    }
    e1000_reg_write(E1000_RDBAL, (uint32_t)rx_ring);
    e1000_reg_write(E1000_RDBAH, 0);
    e1000_reg_write(E1000_RDLEN, sizeof(rx_ring));
    e1000_reg_write(E1000_RDH, 0);
    e1000_reg_write(E1000_RDT, RX_COUNT - 1);       // every buffer but one is the card's
    rx_next = 0;
    e1000_reg_write(E1000_RCTL, RCTL_EN | RCTL_BAM | RCTL_SECRC);
}

static void setup_tx(void) {
    for (int i = 0; i < TX_COUNT; i++) {
        tx_ring[i].address = (uint32_t)tx_buffers[i];
        tx_ring[i].status = DESC_DD;                // "done" = free to use
    }
    e1000_reg_write(E1000_TDBAL, (uint32_t)tx_ring);
    e1000_reg_write(E1000_TDBAH, 0);
    e1000_reg_write(E1000_TDLEN, sizeof(tx_ring));
    e1000_reg_write(E1000_TDH, 0);
    e1000_reg_write(E1000_TDT, 0);
    tx_tail = 0;
    e1000_reg_write(E1000_TCTL, TCTL_EN | TCTL_PSP | TCTL_CT | TCTL_COLD);
    e1000_reg_write(E1000_TIPG, TIPG_DEFAULT);
}

int e1000_init(void) {
    static const uint16_t ids[] = { 0x100E, 0x100F, 0x1004 };   // 82540EM, 82545EM, 82543GC
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]) && !pci; i++)
        pci = pci_find(INTEL_VENDOR, ids[i]);
    if (!pci)
        return 0;

    // DMA buffers must be reachable by their physical address.
    if ((uint32_t)&rx_buffers[RX_COUNT] > IDENTITY_MAP_END || (pci->bar[0] & 1))
        return 0;

    pci_enable(pci, PCI_CMD_MEMORY | PCI_CMD_BUS_MASTER);
    if (!map_registers(pci->bar[0] & ~0xFu))
        return 0;

    reset();
    read_mac();
    setup_rx();
    setup_tx();

    format(bus_info, sizeof(bus_info), "%02x:%02x.%u", pci->bus, pci->device, pci->function);
    register_interrupt_handler(IRQ_BASE + pci->irq, e1000_interrupt);
    e1000_reg_write(E1000_IMS, INT_LSC | INT_RXT0 | INT_RXDMT0 | INT_RXO);
    e1000_reg_read(E1000_ICR);
    pic_unmask(pci->irq);
    return 1;
}

/* ---------- Questions ---------- */

int e1000_present(void)          { return mmio != 0; }
const char *e1000_bus_info(void) { return bus_info; }
uint8_t e1000_irq(void)          { return pci ? pci->irq : 0; }
const e1000_stats_t *e1000_stats(void) { return &stats; }

void e1000_mac(uint8_t out[E1000_MAC_LEN]) {
    memcpy(out, mac, E1000_MAC_LEN);
}

int e1000_link_up(void) {
    return mmio && (e1000_reg_read(E1000_STATUS) & STATUS_LU);
}

int e1000_speed(void) {
    static const int speeds[4] = { 10, 100, 1000, 1000 };
    return mmio ? speeds[(e1000_reg_read(E1000_STATUS) >> STATUS_SPEED_SHIFT) & 3] : 0;
}

int e1000_full_duplex(void) {
    return mmio && (e1000_reg_read(E1000_STATUS) & STATUS_FD);
}

void e1000_set_mac(const uint8_t new_mac[E1000_MAC_LEN]) {
    if (!mmio)
        return;
    memcpy(mac, new_mac, E1000_MAC_LEN);
    program_filter();
}

e1000_receiver_t e1000_set_receiver(e1000_receiver_t rx) {
    uint32_t flags = cpu_irq_save();
    e1000_receiver_t old = receiver;
    receiver = rx;
    cpu_irq_restore(flags);
    return old;
}

void e1000_set_promiscuous(int on) {
    if (!mmio)
        return;
    uint32_t rctl = e1000_reg_read(E1000_RCTL);
    rctl = on ? rctl | RCTL_UPE | RCTL_MPE : rctl & ~(RCTL_UPE | RCTL_MPE);
    e1000_reg_write(E1000_RCTL, rctl);
}

int e1000_promiscuous(void) {
    return mmio && (e1000_reg_read(E1000_RCTL) & RCTL_UPE);
}
