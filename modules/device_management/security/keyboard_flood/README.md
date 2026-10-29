# keyboard_flood -> key_queue_bound

**Attack.** The keyboard IRQ adds a key per press; the shell reads them
later. Type faster than they are read and an unbounded buffer would
overrun into kernel memory.

**Defence (key_queue_bound).** `enqueue()` in `keyboard.c` asks
`def_key_queue_has_room()` before storing a key; a full ring buffer
drops the newest key instead of overwriting. Keystrokes can be lost
under a flood, but memory is never corrupted. (No audit log: this runs
in the interrupt, where logging is unsafe — the verdict comes from the
selftest.)
