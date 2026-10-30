#ifndef MOUSE_H
#define MOUSE_H

/*
 * mouse.h - PS/2 mouse, IRQ 12.
 *
 * The mouse sends 3-byte packets (4 with a scroll wheel):
 *
 *   byte 0: flags   bit 0 left, 1 right, 2 middle button,
 *                   bit 3 ALWAYS 1, bit 4/5 sign of dx/dy,
 *                   bit 6/7 dx/dy overflowed
 *   byte 1: dx      movement right (9-bit signed with bit 4)
 *   byte 2: dy      movement UP    (9-bit signed with bit 5)
 *   byte 3: wheel   (wheel mice only)
 */

#include <stdint.h>

typedef struct {
    int x, y;                       // pointer position on the GUI screen
    int left, right, middle;        // 1 = button held
} mouse_state_t;

// Set up the mouse. Returns 1 if a mouse answered, 0 if there is none.
int init_mouse(void);
int mouse_is_available(void);

void mouse_get_state(mouse_state_t *out);

// The screen is now 'width' x 'height' pixels: keep the pointer on it.
// (bga_set_video_mode calls this whenever the size changes.)
void mouse_set_screen(int width, int height);

// The last pixel the pointer may reach (1023 and 767 at 1024 x 768).
int mouse_max_x(void);
int mouse_max_y(void);

/* ---------- For tests (seclab) ---------- */

// Feed one byte to the packet decoder, as the interrupt does.
void mouse_handle_byte(uint8_t byte);
// Forget any half-received packet.
void mouse_reset_decoder(void);
// Put the pointer back where a test found it.
void mouse_set_state(const mouse_state_t *state);
// 3 or 4 bytes per packet.
int mouse_packet_size(void);

#endif
