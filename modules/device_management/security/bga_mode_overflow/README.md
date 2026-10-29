# bga_mode_overflow -> mode_bounds

**Attack.** `bga_set_video_mode(w, h, bpp)` switches the graphics card's
mode. Other code sizes the framebuffer as `w * h * bpp/8`. A mode like
65535x65535x32 overflows that math; a zero width makes a zero-size
buffer. Later draws then run off the end.

**Defence (mode_bounds).** `bga_set_video_mode()` now asks
`def_bga_mode_ok` first: width/height must be non-zero and within the
card's maximum, and bpp a real depth (15/16/24/32). This check did not
exist before — the attack is what revealed the gap.
