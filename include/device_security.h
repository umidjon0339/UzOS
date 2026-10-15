#ifndef DEVICE_SECURITY_H
#define DEVICE_SECURITY_H

#include <stdint.h>

#define PIT_INPUT_HZ 1193180u
#define PIT_MIN_FREQUENCY ((PIT_INPUT_HZ + 65534u) / 65535u)
#define MOUSE_SCREEN_MAX_X 1023
#define MOUSE_SCREEN_MAX_Y 767

int device_timer_frequency_valid(uint32_t frequency);
int device_clamp_coordinate(int value, int maximum);

#endif
