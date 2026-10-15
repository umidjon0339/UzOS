# Attack: user pointer wrap

**Breaks:** `trust` · **Severity:** HIGH · **Paired defence:** [`user_ptr_bounds`](../../defence/user_ptr_bounds/README.md)

## The issue

A user buffer must remain below `KERNEL_SPACE_START`. Adding a length to
an unchecked address can wrap the address space or cross into kernel
memory, letting a syscall trust a range that is not wholly user-owned.

## How to try it

```text
seclab attack user_ptr_wrap
seclab attack user_ptr_wrap live
seclab verify user_ptr_bounds
```

The probes test the last user byte, a boundary crossing, and a wrapped
32-bit address. They never dereference candidate pointers.
