# toctou_race -> atomic_check

**Attack.** TOCTOU = Time Of Check to Time Of Use. Check "is this file
mine?", and in the gap before you open it an attacker swaps it for a
symlink to a secret - which you then open with your own rights.

**Defence (atomic_check).** Never split the check from the use: do both
in one step the kernel can't be interrupted in (open with
`O_CREAT|O_EXCL`, then act on the file descriptor, not the name). No gap,
no race.
