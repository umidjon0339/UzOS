# Attack: kernel_patch

| | |
|---|---|
| **Stopped by** | [`integrity_check`](../../defence/integrity_check/) |
| **Breaks** | integrity: the kernel runs the code that was built |
| **Severity** | critical |

## What it does

Code in ring 0 can write anywhere, also into the kernel's own
instructions. UzOS maps its code writable, and `exec` programs run in
ring 0, so a malicious program could do it. One byte is enough:

```
before:  test eax, eax ; jz refuse      "if (!allowed) refuse"
after:   test eax, eax ; nop  nop       "always allowed"
```

That is what a rootkit does: it does not break a rule, it rewrites the
code that checks the rule.

The attack patches something harmless on purpose:
`atk_kernel_patch_target()`, a function that nothing ever calls. It
writes `int3` (`0xCC`) over its first byte (`ret`, `0xC3`). Nothing
behaves differently, so only a hash of the code can notice.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the target's address and byte, and measures the kernel |
| `live` | 1. patches the byte  2. starts `/hello` the way `exec` does  3. puts the byte back |

Expected:

```
patched 0x11ca10: c3 -> cc
starting /hello on the patched kernel:
elf: refused: the kernel's code changed since boot (see `integrity`)
byte put back: kernel unchanged again
```

`cleanup()` puts the byte back if a run was cut short.

## Uses

- `integrity_verify()` from [core/integrity](../../../core/integrity/)
- `elf_load_and_run()` and `process_kill()` from process_management

## Try it

```
su
integrity
seclab attack kernel_patch live
audit
```
