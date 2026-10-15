#include "mouse.h"
#include "io.h"
#include "isr.h"
#include "pic.h"
#include "device_security.h"

int mouse_x = 512;
int mouse_y = 384;
int mouse_b_left = 0;
int mouse_b_right = 0;
int mouse_b_middle = 0;

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

// Wait for mouse to be ready for read
static void mouse_wait_read(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 1) == 1) return;
    }
}

// Wait for mouse to be ready for write
static void mouse_wait_write(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(0x64) & 2) == 0) return;
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait_write();
    outb(0x64, 0xD4);
    mouse_wait_write();
    outb(0x60, a_write);
}

static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(0x60);
}

static void mouse_callback(registers_t *regs) {
    (void)regs;
    
    // Always read 0x64 to check status
    uint8_t status = inb(0x64);
    
    // Check if there is data in the buffer
    if (!(status & 1)) {
        pic_send_eoi(12);
        return;
    }
    
    // Read the data byte to clear the buffer (CRITICAL!)
    uint8_t data = inb(0x60);
    
    // If it's not a mouse packet (bit 5), just ignore it but we successfully cleared it
    if (!(status & 0x20)) {
        pic_send_eoi(12);
        return;
    }
    
    mouse_byte[mouse_cycle++] = data;
    
    if (mouse_cycle == 3) {
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
    }
    
    pic_send_eoi(12);
}

void init_mouse(void) {
    uint8_t _status;
    
    // Enable auxiliary mouse device
    mouse_wait_write();
    outb(0x64, 0xA8);
    
    // Enable interrupts
    mouse_wait_write();
    outb(0x64, 0x20);
    mouse_wait_read();
    _status = (inb(0x60) | 2);
    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, _status);
    
    // Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge
    
    // Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // Acknowledge
    
    // IRQ12 is remapped to interrupt 44 (32 + 12)
    register_interrupt_handler(44, mouse_callback);
}
