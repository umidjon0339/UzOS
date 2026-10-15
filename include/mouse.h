#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

extern int mouse_x;
extern int mouse_y;
extern int mouse_b_left;
extern int mouse_b_right;
extern int mouse_b_middle;

int init_mouse(void);

#endif
