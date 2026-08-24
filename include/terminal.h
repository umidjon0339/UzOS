#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>
#include <stdint.h>

void terminal_initialize(uint32_t* fb, uint32_t width, uint32_t height,
                         uint32_t pitch, uint8_t bpp);
void terminal_print(const char* str);
void terminal_putchar(char c);
void terminal_clear(void);

#endif
