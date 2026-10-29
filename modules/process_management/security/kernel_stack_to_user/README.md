# Attack: kernel_stack_to_user

| | |
|---|---|
| **Stopped by** | [`user_stack`](../../defence/user_stack/) |
| **Breaks** | privilege: user code cannot write kernel memory |
| **Severity** | critical |

## What it does

`switch_to_user_mode(eip, esp)` drops the CPU to ring 3 with the stack
pointer it is given. User code then pushes and writes to that stack
freely. If `esp` points into a **kernel** stack, user code can overwrite
the kernel's saved registers and return addresses, and so make the
kernel run its code.

The first version of `testuser` did exactly this: it passed its own
kernel `ESP` to ring 3.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | prints three kernel addresses an attacker could pass |
| `live` | offers each one to the guard as a "user stack" (it never really enters ring 3 with them) |

With `user_stack` on, all three are refused. `testuser` maps a real user
stack page at `0x40001000` instead.

## Uses

`def_user_stack_ok()` from defence/user_stack,
`cpu_read_esp()` from arch/x86/cpu, `stack_top` from boot/boot.s.

## Try it

```
seclab attack kernel_stack_to_user live
testuser                         # the correct way: a real user stack
```
