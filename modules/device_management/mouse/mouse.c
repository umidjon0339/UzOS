/*
 * mouse.c - Talk to the PS/2 mouse and decode its packets.
 *
 * The mouse sits behind the same PS/2 controller as the keyboard:
 * port 0x64 is status/command, port 0x60 is data. Bytes for the mouse
 * are sent with the controller command 0xD4 ("next byte is for the mouse").
 *
 * Uses: inb/outb (arch/x86/io), register_interrupt_handler (arch/x86/isr),
 *       pic_unmask (pic/), vga_scroll_view (vga/) for the wheel, and two
 *       defences: mouse_packet_sync and mouse_bounds.
 */

#include "mouse.h"
#include "io.h"
#include "isr.h"
#include "pic.h"
#include "cpu.h"
#include "vga.h"
#include "def_mouse_packet_sync.h"
#include "def_mouse_bounds.h"

#define MOUSE_IRQ        12
#define PS2_DATA         0x60
#define PS2_STATUS       0x64       // read
#define PS2_COMMAND      0x64       // write

#define STATUS_OUTPUT    0x01       // a byte is waiting at port 0x60
#define STATUS_INPUT     0x02       // controller busy: don't write yet
#define STATUS_FROM_AUX  0x20       // the waiting byte came from the mouse

#define CMD_ENABLE_AUX   0xA8       // controller: turn the mouse port on
#define CMD_READ_CONFIG  0x20
#define CMD_WRITE_CONFIG 0x60
#define CMD_TO_MOUSE     0xD4       // controller: next data byte is for the mouse
#define CONFIG_AUX_IRQ   0x02       // config bit: raise IRQ 12 for mouse data

#define MOUSE_DEFAULTS   0xF6
#define MOUSE_ENABLE     0xF4       // start sending packets
#define MOUSE_SET_RATE   0xF3
#define MOUSE_GET_ID     0xF2
#define MOUSE_ACK        0xFA

#define WAIT_LOOPS       100000

static mouse_state_t state = { 512, 384, 0, 0, 0 };   // start in the middle
static uint8_t packet[4];
static int cycle;                   // which byte of the packet comes next
static int packet_size = 3;
static int available;

/* ---------- Controller I/O ---------- */

static int wait_can_write(void) {
    for (int i = 0; i < WAIT_LOOPS; i++)
        if (!(inb(PS2_STATUS) & STATUS_INPUT))
            return 1;
    return 0;
}

static int wait_can_read(void) {
    for (int i = 0; i < WAIT_LOOPS; i++)
        if (inb(PS2_STATUS) & STATUS_OUTPUT)
            return 1;
    return 0;
}

static int controller_command(uint8_t cmd) {
    if (!wait_can_write()) return 0;
    outb(PS2_COMMAND, cmd);
    return 1;
}

static int read_byte(uint8_t *value) {
    if (!wait_can_read()) return 0;
    *value = inb(PS2_DATA);
    return 1;
}

static int write_byte(uint8_t value) {
    if (!wait_can_write()) return 0;
    outb(PS2_DATA, value);
    return 1;
}

// Send a byte to the mouse and expect "ACK" back.
static int mouse_command(uint8_t value) {
    uint8_t reply;
    return controller_command(CMD_TO_MOUSE) && write_byte(value) &&
           read_byte(&reply) && reply == MOUSE_ACK;
}

// The "IntelliMouse" knock: rates 200, 100, 80, then the ID becomes 3.
static int enable_wheel(void) {
    uint8_t id;
    return mouse_command(MOUSE_SET_RATE) && mouse_command(200) &&
           mouse_command(MOUSE_SET_RATE) && mouse_command(100) &&
           mouse_command(MOUSE_SET_RATE) && mouse_command(80) &&
           mouse_command(MOUSE_GET_ID) && read_byte(&id) && (id == 3 || id == 4);
}

/* ---------- Packet decoding ---------- */

static void apply_packet(void) {
    // DEFENCE mouse_packet_sync: overflowed deltas are meaningless; drop.
    if (!def_mouse_packet_ok(packet))
        return;

    // dx/dy are 9-bit numbers: byte + sign bit from the flags.
    int dx = packet[1] - ((packet[0] & 0x10) ? 256 : 0);
    int dy = packet[2] - ((packet[0] & 0x20) ? 256 : 0);

    // DEFENCE mouse_bounds: the pointer never leaves the screen.
    state.x = def_mouse_clamp(state.x + dx, MOUSE_SCREEN_MAX_X);
    state.y = def_mouse_clamp(state.y - dy, MOUSE_SCREEN_MAX_Y);   // PS/2 dy points up
    state.left   = (packet[0] >> 0) & 1;
    state.right  = (packet[0] >> 1) & 1;
    state.middle = (packet[0] >> 2) & 1;

    if (packet_size == 4) {
        int wheel = packet[3] & 0x0F;       // 4-bit signed
        if (wheel & 0x08)
            wheel -= 16;
        if (wheel)
            vga_scroll_view(-wheel);        // wheel scrolls the text console
    }
}

void mouse_handle_byte(uint8_t byte) {
    // DEFENCE mouse_packet_sync: a packet must start with bit 3 set.
    // If it doesn't, we are out of step: drop bytes until we are back.
    if (cycle == 0 && !def_mouse_sync_ok(byte))
        return;

    packet[cycle++] = byte;
    if (cycle == packet_size) {
        cycle = 0;
        apply_packet();
    }
}

static void mouse_callback(registers_t *regs) {
    (void)regs;
    uint8_t status = inb(PS2_STATUS);

    // The arch layer already sends the EOI; we only read the byte.
    if ((status & STATUS_OUTPUT) && (status & STATUS_FROM_AUX))
        mouse_handle_byte(inb(PS2_DATA));
}

/* ---------- Setup ---------- */

int init_mouse(void) {
    uint8_t config;

    // Turn the mouse port on and let it raise IRQ 12.
    if (!controller_command(CMD_ENABLE_AUX)) return 0;
    if (!controller_command(CMD_READ_CONFIG) || !read_byte(&config)) return 0;
    if (!controller_command(CMD_WRITE_CONFIG) || !write_byte(config | CONFIG_AUX_IRQ)) return 0;

    if (!mouse_command(MOUSE_DEFAULTS))
        return 0;
    packet_size = enable_wheel() ? 4 : 3;

    register_interrupt_handler(32 + MOUSE_IRQ, mouse_callback);
    available = mouse_command(MOUSE_ENABLE);
    if (available)
        pic_unmask(MOUSE_IRQ);
    return available;
}

int mouse_is_available(void) { return available; }
int mouse_packet_size(void)  { return packet_size; }

void mouse_get_state(mouse_state_t *out) {
    uint32_t irq = cpu_irq_save();      // copy all fields from one moment
    *out = state;
    cpu_irq_restore(irq);
}

void mouse_set_state(const mouse_state_t *s) {
    uint32_t irq = cpu_irq_save();
    state = *s;
    cpu_irq_restore(irq);
}

void mouse_reset_decoder(void) {
    cycle = 0;
}
