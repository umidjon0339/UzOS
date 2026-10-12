#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint32_t timer_ticks;

void init_timer(uint32_t frequency);
void timer_wait(uint32_t ticks);

#endif
