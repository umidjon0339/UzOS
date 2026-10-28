# meltdown -> page_separation  (illustration)

**Attack (illustration).** Meltdown reads a kernel byte the page tables
forbid, in the window before the CPU raises the fault, and leaks it
through cache timing. It needs a specific out-of-order CPU; UzOS is a
simple 32-bit kernel and **cannot run a real Meltdown**. We illustrate
the line it attacks, not a working exploit.

**Defence (page_separation).** A kernel page is present but has no
PAGE_USER bit, so a ring 3 read faults. The CPU enforces this on every
access; UzOS sets the bit when it maps memory (core/vmm, core/umem).
Real systems keep this line with KPTI and microcode; here the lesson is
the U/S bit itself.
