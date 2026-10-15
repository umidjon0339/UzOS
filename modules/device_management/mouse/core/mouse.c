#include "mouse.h"
#include "io.h"
#include "isr.h"
#include "pic.h"
#include "device_security.h"
#include "vga.h"

int mouse_x = 512;
int mouse_y = 384;
int mouse_b_left = 0;
int mouse_b_right = 0;
int mouse_b_middle = 0;

static uint8_t mouse_cycle;
static uint8_t mouse_byte[4];
static uint8_t mouse_packet_size = 3;

// Wait for mouse to be ready for read
static int mouse_wait_read(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 1) == 1) return 1;
    }
    return 0;
}

// Wait for mouse to be ready for write
static int mouse_wait_write(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 2) == 0) return 1;
    }
    return 0;
}

static int mouse_write(uint8_t value) {
    if (!mouse_wait_write()) return 0;
    outb(0x64, 0xD4);
    if (!mouse_wait_write()) return 0;
    outb(0x60, value);
    return 1;
}

static int mouse_read(uint8_t *value) {
    if (!mouse_wait_read()) return 0;
    *value = inb(0x60);
    return 1;
}

static int mouse_command(uint8_t command) {
    uint8_t response;
    return mouse_write(command) && mouse_read(&response) && response == 0xFA;
}

static int mouse_set_sample_rate(uint8_t rate) {
    return mouse_command(0xF3) && mouse_command(rate);
}

static void mouse_callback(registers_t *regs) {
    (void)regs;
    
    // Always read 0x64 to check status
    uint8_t status = inb(0x64);
    
    if (!(status & 1) || !(status & 0x20)) {
        pic_send_eoi(12);
        return;
    }
    
    uint8_t data = inb(0x60);

    mouse_byte[mouse_cycle++] = data;
    
    if (mouse_cycle == mouse_packet_size) {
        mouse_cycle = 0;
        
        // Parse the packet
        int state = mouse_byte[0];
        int d_x = mouse_byte[1];
        int d_y = mouse_byte[2];
        
        // Sign extension for X
        if (state & (1 << 4)) {
            d_x |= 0xFFFFFF00;
        }
        
        // Sign extension for Y
        if (state & (1 << 5)) {
            d_y |= 0xFFFFFF00;
        }
        
        // Update state
        mouse_x += d_x;
        mouse_y -= d_y; // Y is inverted in PS/2 mouse
        
        // Clamp to screen bounds (assuming 1024x768)
        mouse_x = device_clamp_coordinate(mouse_x, MOUSE_SCREEN_MAX_X);
        mouse_y = device_clamp_coordinate(mouse_y, MOUSE_SCREEN_MAX_Y);
        
        mouse_b_left = state & 0x01;
        mouse_b_right = (state & 0x02) >> 1;
        mouse_b_middle = (state & 0x04) >> 2;

        if (mouse_packet_size == 4) {
            int wheel = mouse_byte[3] & 0x0F;
            if (wheel & 0x08) wheel -= 16;
            if (wheel) vga_scroll_view(-wheel);
        }
    }
    
    pic_send_eoi(12);
}

int init_mouse(void) {
    uint8_t _status;
    uint8_t device_id = 0;

    mouse_cycle = 0;
    mouse_packet_size = 3;
    
    // Enable auxiliary mouse device
    if (!mouse_wait_write()) return 0;
    outb(0x64, 0xA8);
    
    // Enable interrupts
    if (!mouse_wait_write()) return 0;
    outb(0x64, 0x20);
    if (!mouse_wait_read()) return 0;
    _status = (inb(0x60) | 2);
    if (!mouse_wait_write()) return 0;
    outb(0x64, 0x60);
    if (!mouse_wait_write()) return 0;
    outb(0x60, _status);
    
    int initialized = mouse_command(0xF6);
    if (initialized &&
        mouse_set_sample_rate(200) &&
        mouse_set_sample_rate(100) &&
        mouse_set_sample_rate(80) &&
        mouse_write(0xF2) &&
        mouse_read(&device_id) &&
        device_id == 0xFA &&
        mouse_read(&device_id) &&
        (device_id == 3 || device_id == 4)) {
        mouse_packet_size = 4;
    }

    register_interrupt_handler(44, mouse_callback);
    return mouse_command(0xF4) && initialized;
}
