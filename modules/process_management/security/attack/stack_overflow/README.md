# Attack: stack_overflow

| | |
|---|---|
| **Stopped by** | [`stack_guard`](../../defence/stack_guard/) |
| **Breaks** | isolation: a process stays inside its own memory |
| **Severity** | critical |

## What it does

Every process gets a 4 KiB stack from the kernel heap. Each function
call pushes a frame. Deep recursion keeps pushing below the stack's
bottom, and the x86 CPU does not stop it: the next heap object is
silently overwritten. Nothing crashes right away, which makes this
worse than a crash.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the current stack and the bytes left below `ESP` |
| `live` | measures one frame with an 8-deep recursion, then asks the guard about an `ESP` one push below a stack's bottom |

`live` never really overflows a stack, since that would corrupt the
heap. It shows that the check the scheduler runs catches it.

## Uses

`def_stack_guard_check()` from security/defence/stack_guard,
`scheduler_get_current_process()` from core/scheduler.

## Try it

```
seclab attack stack_overflow live
seclab log
```
