# Defence: user pointer bounds

**Counters:** [`user_ptr_wrap`](../../attack/user_ptr_wrap/README.md)

The policy rejects null pointers, addresses in kernel space, and ranges
whose size exceeds the bytes remaining before the kernel boundary. It
uses subtraction rather than `start + size`, so the check itself cannot
overflow.

`sys_munmap()` checks the user range before releasing physical frames.
The same policy is covered by the defence selftest:

```text
seclab verify user_ptr_bounds
seclab attack user_ptr_wrap live
```
