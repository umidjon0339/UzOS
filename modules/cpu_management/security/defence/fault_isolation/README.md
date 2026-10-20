# Defence: fault_isolation

| | |
|---|---|
| **Stops** | [`user_exception`](../../attack/user_exception/) |
| **Protects** | isolation: one program's crash cannot stop the others |
| **Called from** | `fault_handler()` in [core/faults](../../../core/faults/cpu_faults.c) and the page fault handler in [vmm.c](../../../../memory_management/core/vmm/vmm.c) |

## The rule

```
survivable = (saved CS & 3) == 3                  the fault came from ring 3
         and vector is not 2, 8 or 18             not NMI / double fault / machine check

survivable  -> print a report, kill the process, the OS runs on
otherwise   -> isr_panic(): print the registers, halt
```

## Why

When the CPU raises an exception it pushes CS, the code segment that was
running. Its low 2 bits are the **ring** of the code that faulted:

| Saved CS | Ring | Who is broken? | What we do |
|---|---|---|---|
| `0x1B` | 3 | the program | kill it, like Linux sends SIGSEGV/SIGFPE |
| `0x08` | 0 | the kernel | halt: going on could corrupt everything |

Before this defence, only page faults had a handler. Every other
exception reached `isr_panic()`, so **any** user program could stop the
whole OS with a single `div` by zero, `ud2` or `cli`.

Vectors 2, 8 and 18 are never survivable: they say the machine itself
(or the kernel's fault handling) is broken, not one program.

## Uses

- `registers_t` (saved `cs`, `int_no`) from arch/x86/isr
- `sec_log()` from the security lab

## Try it

```
seclab attack user_exception live
faults                           # every program was killed, the shell still works
seclab verify fault_isolation
```
