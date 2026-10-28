# spectre -> user_kernel_isolation  (illustration)

**Attack (illustration).** Spectre makes the CPU execute past a check
speculatively, reads a forbidden value in that window, and leaks it by
cache timing. It needs branch prediction and out-of-order execution to
train and measure; UzOS has neither, so it **cannot run a real
Spectre**.

**Defence (user_kernel_isolation).** Kernel memory is mapped without the
user bit, so neither a normal nor a speculative ring 3 access may read
it - the boundary Spectre tries to cross. Real mitigations add microcode
and speculation barriers on top of this architectural line.
