#ifndef DEF_INPUT_BOUND_H
#define DEF_INPUT_BOUND_H

// Is there room for one more character in a line of 'capacity' bytes
// holding 'used' characters (plus the final NUL)? 1 = yes, 0 = full (logged).
int def_input_room_ok(int used, int capacity);

#endif
