# process_hijacking -> control_flow_integrity  (concept)

**Attack (concept).** Redirect a running program to attacker-chosen code
by corrupting a return address or function pointer.

**Defence (control_flow_integrity).** True CFI - checking every indirect
call/return against a legal target - is a compiler and hardware feature.
UzOS's part is isolation: the kernel owns each process's entry and
ring-3 return frames, out of ring 3's reach.
