# rowhammer -> ecc_and_refresh  (concept)

**Attack (concept).** Hammer one DRAM row thousands of times until the
disturbance flips a bit in a neighbouring row - memory you were never
allowed to write. It is a physical property of real DRAM.

**Defence (concept).** ECC memory, higher refresh rates and Target Row
Refresh, all in the memory controller - not the OS. UzOS runs on
emulated memory that cannot flip bits, so there is nothing to reproduce
or enforce here.
