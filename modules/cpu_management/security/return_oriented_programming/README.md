# return_oriented_programming -> wx_and_aslr  (concept)

**Attack (concept).** Overflow a stack with a list of return addresses,
each pointing at a short tail of existing code (a "gadget") ending in
`ret`. The CPU runs gadget after gadget - a new program built from old
code, injecting nothing.

**Defence (concept).** W^X (no page both writable and executable), ASLR
(unpredictable addresses) and CFI (returns must land on real call
sites). **UzOS cannot enforce W^X**: 32-bit paging without PAE has no
no-execute bit. This pair teaches the defences and is honest that this
kernel's page model (`PAGE_WRITABLE`, `PAGE_USER`, no NX) cannot run them.
