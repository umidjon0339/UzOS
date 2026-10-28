# code_injection -> nx_pages  (concept)

**Attack (concept).** Write machine code into a data area you control (a
buffer, the stack), then trick the program into jumping to it - your
bytes run with the victim's rights.

**Defence (concept).** NX / W^X: data pages are non-executable, so the
jump faults and injected bytes become a crash, not code. **UzOS cannot
mark a page non-executable** (32-bit paging without PAE has no NX bit),
so this is a concept. Its code-reuse cousin, which needs no injection at
all, is `return_oriented_programming`.
