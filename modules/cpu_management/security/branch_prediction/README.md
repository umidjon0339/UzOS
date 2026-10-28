# branch_prediction -> speculation_barrier  (illustration)

**Attack (illustration).** Train the CPU's branch predictor to guess
wrong so a victim speculatively runs a path that touches a secret, then
read it from the cache (Spectre v2's technique). It needs a shared,
trainable predictor and out-of-order execution; UzOS has neither.

**Defence (speculation_barrier).** The structural fixes are **hardware
and toolchain** - microcode (IBRS/IBPB) and retpolines - not something a
teaching kernel can add. What UzOS provides is the isolation that bounds
any leak: one task at a time, kernel pages hidden from ring 3. The
lesson is where the real fix lives.
