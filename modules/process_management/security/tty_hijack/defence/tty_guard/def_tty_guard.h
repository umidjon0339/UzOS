#ifndef DEF_TTY_GUARD_H
#define DEF_TTY_GUARD_H

#include <stdint.h>

// May a process of session 'caller_sid' push keystrokes into the input
// of the terminal that session 'tty_sid' owns (Linux: TIOCSTI)? Only
// into its OWN terminal, or with CAP_SECLAB-like admin rights; never
// from another session. CONCEPT: UzOS has one console and no way for a
// program to inject input, so nothing calls this yet.
int def_tty_inject_ok(uint32_t caller_sid, uint32_t tty_sid, int admin);

#endif
