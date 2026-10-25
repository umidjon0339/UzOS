# Defence: user_stack

| | |
|---|---|
| **Stops** | [`kernel_stack_to_user`](../../attack/kernel_stack_to_user/) |
| **Protects** | privilege: user code cannot write kernel memory |
| **Called from** | `testuser` in [commands/testuser](../../../commands/testuser/cmd_testuser.c), right before `switch_to_user_mode()` |

## The rule

```
USER_SPACE_START < ESP <= USER_SPACE_END      (0x08000000 .. 0xC0000000)
ESP is a multiple of 4
```

## Why

Entering ring 3 means handing the CPU a new `ESP`. User code then
pushes, pops and writes to that stack freely. If `ESP` pointed at a
kernel stack, user code could overwrite saved kernel registers and
return addresses, and so run its own code with kernel rights.

The first version of `testuser` did exactly that: it passed the kernel's
own `ESP` to ring 3. Now `testuser` maps a real user stack page at
`0x40001000` and this guard checks it.

## Uses

- `USER_SPACE_START` / `USER_SPACE_END` from memory_management/core/layout
- `audit_log()` from security_management (core/audit)

## Try it

```
testuser                          # allowed: real user stack
seclab attack kernel_stack_to_user live
seclab verify user_stack
```
