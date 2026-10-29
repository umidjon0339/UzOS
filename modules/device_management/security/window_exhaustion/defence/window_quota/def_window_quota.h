#ifndef DEF_WINDOW_QUOTA_H
#define DEF_WINDOW_QUOTA_H

// May another window be created, given 'used' windows already open? The
// window manager has a fixed number of slots (MAX_WINDOWS). wm_create_
// window() asks this before taking a slot, so a flood of windows cannot
// overrun the table. 1 = room, 0 = refused (logged).
int def_window_quota_ok(int used);

#endif
