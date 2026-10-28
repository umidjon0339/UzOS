#ifndef DEF_SPECULATION_BARRIER_H
#define DEF_SPECULATION_BARRIER_H

// 1 if what a branch-prediction attack could leak is already bounded in
// UzOS: there is no shared speculative state to train (one CPU, one
// task at a time) and the user/kernel page boundary still holds. Real
// CPUs need microcode (IBRS/IBPB) and retpolines on top; those are
// hardware/toolchain fixes, not something a teaching kernel can add.
int def_speculation_bounded(void);

#endif
