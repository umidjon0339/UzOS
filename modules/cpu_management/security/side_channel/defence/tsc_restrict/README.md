# Defence: tsc_restrict

| | |
|---|---|
| **Stops** | [`tsc_timing`](../../attack/tsc_timing/) |
| **Protects** | disclosure: no secrets leak through timing |
| **Called from** | `tsc_init()` in [core/tsc](../../../core/tsc/tsc.c) (switch on), `fault_handler()` in [core/faults](../../../core/faults/cpu_faults.c) (report) |

## The rule

```
CR4.TSD = 1        Time Stamp Disable

rdtsc in ring 0  -> works (the kernel still measures, cpustat, cpuinfo)
rdtsc in ring 3  -> #13 General Protection -> fault_isolation kills it
```

## Why

`rdtsc` returns the number of CPU cycles since power on. At 2 GHz it
moves 2 billion times a second: fine enough to see how long **one
kernel function** took.

That is the basis of a **timing attack**. If the kernel does a little
more work in one case than another (a file exists or not, a password's
first letter is right or not), a program that can measure it learns the
answer without being told.

| Clock a ring 3 program has | Moves every | Enough for a timing attack? |
|---|---|---|
| `rdtsc` (before this defence) | < 1 ns | yes |
| `uptime()` syscall | 10 ms | no |

Real systems: Linux leaves `rdtsc` open for speed, but `prctl(PR_SET_TSC)`
and seccomp can close it; browsers made their clocks coarser after the
Spectre attacks for the same reason.

## Uses

- `cpu_read_cr4()` / `cpu_write_cr4()` / `CR4_TSD` from arch/x86/cpu
- `cpu_has(CPU_FEAT_TSC)` from [core/info](../../../core/info/)
- `audit_log()` from security_management (core/audit)

## Try it

```
cpuinfo                          # CR4 TSD on
seclab attack tsc_timing live
seclab verify tsc_restrict
```
