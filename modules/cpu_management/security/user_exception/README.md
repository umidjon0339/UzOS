# Attack: user_exception

| | |
|---|---|
| **Stopped by** | [`fault_isolation`](../../defence/fault_isolation/) |
| **Breaks** | isolation: one program's crash must not stop the others |
| **Severity** | critical |

## What it does

Five tiny ring 3 programs ([user_exception.s](user_exception.s)), each
running one instruction the CPU refuses:

| Program | Instruction | Exception | Why the CPU says no |
|---|---|---|---|
| `div0` | `div ecx` with ecx = 0 | #0 Division By Zero | no answer exists |
| `ud2` | `ud2` | #6 Invalid Opcode | undefined on purpose |
| `cli` | `cli` | #13 General Protection | interrupts are ring 0's business (IOPL 0) |
| `in` | `in al, 0x60` | #13 General Protection | device ports are ring 0 only |
| `hlt` | `hlt` | #13 General Protection | only the kernel may stop the CPU |

No special rights needed: **any** user program can do this.

Without a handler, every one of these ends in `isr_panic()`:
"System Halted!". One user could stop the machine for everybody.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the programs and the exception each one raises |
| `live` | runs all five as user 1 in ring 3; each is killed with a `[CPU FAULT]` report, the shell keeps working |

## Uses

- `usertask_start()` / `usertask_wait()` from process_management/core/usertask
- `cpu_fault_total()` / `cpu_fault_log_get()` from [core/faults](../../../core/faults/)

## Try it

```
seclab attack user_exception live
faults
```
