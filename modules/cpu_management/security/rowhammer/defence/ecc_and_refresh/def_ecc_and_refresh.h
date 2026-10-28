#ifndef DEF_ECC_AND_REFRESH_H
#define DEF_ECC_AND_REFRESH_H

// 1 if bit flips from Rowhammer cannot occur here: UzOS runs on emulated
// memory (QEMU) that does not model DRAM charge leakage. On real
// hardware the defences are ECC memory, higher refresh rates and
// Target Row Refresh - all in the memory controller, not the OS.
int def_memory_not_vulnerable(void);

#endif
