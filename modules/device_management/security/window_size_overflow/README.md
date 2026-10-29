# window_size_overflow -> window_bounds

**Attack.** `wm_create_window` allocates its canvas with
`kmalloc(w * h * 4)`. A size like 100000x100000 overflows that multiply
(kmalloc returns a tiny buffer the first draw overruns) or drains the
heap.

**Defence (window_bounds).** `wm_create_window()` now asks
`def_window_size_ok` first: width/height must be positive and no bigger
than the screen. Like bga_mode_overflow, this check did not exist before
— the attack revealed the gap.
