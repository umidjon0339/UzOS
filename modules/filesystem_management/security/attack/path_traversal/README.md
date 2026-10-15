# Attack: path traversal

**Breaks:** `trust` · **Severity:** HIGH · **Paired defence:** [`path_component`](../../defence/path_component/README.md)

Parent components and nested paths can escape the filesystem's
single-component lookup model. The demo probes accepted and rejected
names without opening, creating, or deleting files.

```text
seclab attack path_traversal
seclab attack path_traversal live
seclab verify path_component
```
