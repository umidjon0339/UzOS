#ifndef DEF_CONTROL_FLOW_INTEGRITY_H
#define DEF_CONTROL_FLOW_INTEGRITY_H

// 1 marks what keeps a process's control flow its own in UzOS: the
// kernel sets a process's entry point and ring-3 return frame, and ring
// 3 cannot change kernel memory to redirect them. True CFI (checking
// every indirect call/return against a known target) is a compiler +
// hardware feature, documented here rather than enforced.
int def_entry_is_kernel_set(void);

#endif
