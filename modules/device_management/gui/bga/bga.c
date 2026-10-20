/*
 * bga.c - Switch the BGA card between text mode and a graphics mode.
 *
 * Uses: outw/inw (arch/x86/io), FRAMEBUFFER_START (memory_management
 *       layout), which the VMM maps 1:1 at boot.
 */

#include "bga.h"
#include "io.h"
#include "memory_layout.h"

#define BGA_INDEX_PORT  0x01CE
#define BGA_DATA_PORT   0x01CF

#define REG_ID          0       // card version
#define REG_XRES        1
#define REG_YRES        2
#define REG_BPP         3
#define REG_ENABLE      4

#define ID_FIRST        0xB0C0  // oldest BGA version
#define ID_LAST         0xB0C5  // newest
#define ENABLED         0x01
#define LINEAR_FB       0x40    // use one flat framebuffer, not banks

static void write_reg(uint16_t reg, uint16_t value) {
    outw(BGA_INDEX_PORT, reg);
    outw(BGA_DATA_PORT, value);
}

static uint16_t read_reg(uint16_t reg) {
    outw(BGA_INDEX_PORT, reg);
    return inw(BGA_DATA_PORT);
}

int bga_is_available(void) {
    uint16_t id = read_reg(REG_ID);
    return id >= ID_FIRST && id <= ID_LAST;
}

void bga_set_video_mode(uint16_t width, uint16_t height, uint16_t bpp) {
    write_reg(REG_ENABLE, 0);           // registers may only change while off
    write_reg(REG_XRES, width);
    write_reg(REG_YRES, height);
    write_reg(REG_BPP, bpp);
    write_reg(REG_ENABLE, ENABLED | LINEAR_FB);
}

void bga_disable(void) {
    write_reg(REG_ENABLE, 0);
}

uint32_t *bga_framebuffer(void) {
    return (uint32_t *)FRAMEBUFFER_START;
}
