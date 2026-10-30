# Defence: protected_hardlinks

| | |
|---|---|
| **Stops** | [`hardlink_abuse`](../../) |
| **Protects** | privilege: a user decides nothing about root's files |
| **Called from** | `sys_link()`, `sys_linkat()` |

## The rule

1. A hard link only to a file you own (or `CAP_FILES`).
2. Never to a directory (loops in the tree).

(Linux: `fs.protected_hardlinks = 1`.)

## Try it

```
seclab verify protected_hardlinks
```
