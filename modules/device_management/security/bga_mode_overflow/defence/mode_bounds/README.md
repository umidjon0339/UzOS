
## Checked at build time too

The biggest mode allowed (2560 x 1600 x 4 bytes = 15.6 MiB) must fit the
video memory the VMM maps (`FRAMEBUFFER_START..END`, 16 MiB). If it did
not, drawing the bottom rows would run past the mapping. A
`_Static_assert` in `def_mode_bounds.h` makes the build fail if someone
raises the maximum or shrinks the mapping.
