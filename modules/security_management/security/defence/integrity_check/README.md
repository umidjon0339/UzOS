# Defence: integrity_check

| | |
|---|---|
| **Stops** | [`kernel_patch`](../../attack/kernel_patch/) |
| **Protects** | integrity: the kernel's code and constants stay as built |
| **Called from** | `integrity_verify()` in [core/integrity](../../../core/integrity/), which `elf_load_and_run()` calls before every program, and the `integrity` command |

## The rule

```
1. the region was measured at boot        (its hash is not all zero)
2. SHA-256(region now) == SHA-256(region at boot), all 32 bytes
```

The regions are `.text` (code) and `.rodata` (constants: strings,
tables, the syscall table). A changed kernel starts no more programs;
only a reboot gives back the kernel that was built.

## Why

Every other defence is made of kernel code. If that code can be
changed, every check can be turned into "always yes". A hash cannot
stop the write, but it notices it: change one bit anywhere and the
hash is completely different.

Linux has the same idea: LKRG (Linux Kernel Runtime Guard) hashes the
kernel's code and checks it again on events like starting a program.

## Limits

- It **detects**, it does not prevent: the bytes were already written.
  Preventing needs the code pages mapped read-only and CR0.WP on.
- Only what never changes can be hashed. The IDT, the GDT and the
  process table live in writable data, which is not checked.
- One check takes about 17 ms (177 KB of SHA-256).

## Uses

- `sha256()` from user_management/core/hash
- `audit_log()` from [core/audit](../../../core/audit/)

## Try it

```
integrity
seclab verify integrity_check
su
seclab attack kernel_patch live
```
