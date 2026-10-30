# Defence: kptr_hide

| | |
|---|---|
| **Stops** | [`kernel_pointer_leak`](../../) |
| **Protects** | disclosure: where the kernel keeps its stacks and code |
| **Called from** | `procinfo` (commands/procinfo), for every kernel address it prints |

## The rule

A kernel address is shown only to **CAP_PTRACE** (root). Everyone else
sees `0x00000000`. Linux: `kptr_restrict` and the `%pK` format.

## Try it

```
seclab verify kptr_hide
seclab attack kernel_pointer_leak live
```
