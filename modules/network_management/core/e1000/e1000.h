#ifndef E1000_H
#define E1000_H

/*
 * e1000.h - Driver for the Intel e1000 network card (82540EM).
 *
 * QEMU's and VirtualBox's default network card. It sits on the PCI bus;
 * its registers are 128 KiB of memory ("MMIO", BAR0) that the driver
 * reads and writes like normal variables.
 *
 * Sending and receiving work through two RINGS of descriptors in RAM.
 * A descriptor says "a frame buffer is at this physical address, this
 * long". The card reads and writes RAM itself (DMA, "bus master"):
 *
 *   RX ring                                 TX ring
 *   +---+---+---+---+---+---+               +---+---+---+---+
 *   |   |   |   |   |   |   |               |   |   |   |   |
 *   +---+---+---+---+---+---+               +---+---+---+---+
 *         ^ head (card fills next)                ^ head (card sends next)
 *                     ^ tail (driver gives back)          ^ tail (driver adds here)
 *
 * The card fills an empty RX buffer with an arriving frame, sets the
 * descriptor's DD ("descriptor done") bit and raises its IRQ. The driver
 * takes the frame and hands the buffer back by moving the tail.
 * To send, the driver fills the descriptor at the tail and moves the
 * tail on; the card sends it and sets DD.
 */

#include <stdint.h>

#define E1000_MAC_LEN   6
#define E1000_MAX_FRAME 1518    // biggest Ethernet frame (with the 4-byte CRC)

// Registers: byte offsets into the card's MMIO area (Intel's names).
#define E1000_CTRL    0x0000    // device control: reset, "set link up"
#define E1000_STATUS  0x0008    // link up, speed, duplex
#define E1000_EERD    0x0014    // EEPROM read
#define E1000_MDIC    0x0020    // talk to the PHY
#define E1000_ICR     0x00C0    // interrupt cause (reading clears it)
#define E1000_ICS     0x00C8    // interrupt cause set (raise one by hand)
#define E1000_IMS     0x00D0    // interrupt mask set (enable causes)
#define E1000_IMC     0x00D8    // interrupt mask clear (disable causes)
#define E1000_RCTL    0x0100    // receive control
#define E1000_TCTL    0x0400    // transmit control
#define E1000_TIPG    0x0410    // gap between sent frames
#define E1000_RDBAL   0x2800    // RX ring: physical address, low 32 bits
#define E1000_RDBAH   0x2804    //          high 32 bits
#define E1000_RDLEN   0x2808    //          size in bytes
#define E1000_RDH     0x2810    //          head (the card's position)
#define E1000_RDT     0x2818    //          tail (the driver's position)
#define E1000_TDBAL   0x3800    // TX ring: the same five
#define E1000_TDBAH   0x3804
#define E1000_TDLEN   0x3808
#define E1000_TDH     0x3810
#define E1000_TDT     0x3818
#define E1000_MTA     0x5200    // multicast table, 128 words
#define E1000_RAL     0x5400    // receive address (our MAC), low 4 bytes
#define E1000_RAH     0x5404    //                            high 2 bytes + "valid"

// Counters the card keeps (cleared when read).
#define E1000_CRCERRS 0x4000    // frames with a bad checksum
#define E1000_MPC     0x4010    // missed: no free RX buffer
#define E1000_GPRC    0x4074    // good packets received
#define E1000_GPTC    0x4080    // good packets sent

// Packets and bytes counted by the driver itself.
typedef struct {
    uint32_t rx_packets, rx_bytes, rx_errors, rx_dropped;
    uint32_t tx_packets, tx_bytes, tx_errors;
    uint32_t interrupts, link_changes;
} e1000_stats_t;

// Find the card on the PCI bus and start it. Returns 1 if a card works.
int e1000_init(void);
int e1000_present(void);

// Where the card is, for `ethtool -i`: "00:03.0", IRQ line.
const char *e1000_bus_info(void);
uint8_t e1000_irq(void);

// The card's hardware address (MAC), read from its EEPROM.
void e1000_mac(uint8_t out[E1000_MAC_LEN]);

// The link: is a cable in, how fast (10 / 100 / 1000 Mb/s), full duplex?
int e1000_link_up(void);
int e1000_speed(void);
int e1000_full_duplex(void);

// Send one Ethernet frame (destination MAC first, no CRC: the card adds
// it). Returns 1 if the card took it, 0 if the ring was full or len is bad.
int e1000_send(const void *frame, uint16_t len);

// Who gets each received frame. Called from the card's interrupt.
// Returns the previous receiver (so a test can borrow it).
typedef void (*e1000_receiver_t)(const uint8_t *frame, uint16_t len);
e1000_receiver_t e1000_set_receiver(e1000_receiver_t rx);

// A new MAC: the address we send from and the receive filter's.
void e1000_set_mac(const uint8_t mac[E1000_MAC_LEN]);

// Promiscuous mode: receive frames for every MAC, not only ours (tcpdump).
void e1000_set_promiscuous(int on);
int e1000_promiscuous(void);

const e1000_stats_t *e1000_stats(void);

// Join the stack as interface "eth0" (eth0.c). Returns its index, 0 if no card.
int e1000_register_eth0(void);

/* ---------- Raw access (ethtool -d / -t) ---------- */

// A register, by its E1000_* offset above.
uint32_t e1000_reg_read(uint32_t offset);
void e1000_reg_write(uint32_t offset, uint32_t value);

// One 16-bit word of the EEPROM (word 0-2 = the MAC). -1 if it timed out.
int e1000_eeprom_read(uint8_t word);

// One register of the PHY (the chip that drives the cable). -1 on error.
int e1000_phy_read(uint8_t reg);
int e1000_phy_write(uint8_t reg, uint16_t value);

#endif
