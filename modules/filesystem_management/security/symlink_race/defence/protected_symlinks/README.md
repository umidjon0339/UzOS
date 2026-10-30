# Defence: protected_symlinks

| | |
|---|---|
| **Stops** | [`symlink_race`](../../) |
| **Protects** | trust: root writes where it means to, not where a user points |
| **Called from** | `vfs_lookup()` (core/vfs), before every symbolic link it follows |

## The rule

In a directory others may write to, a symbolic link is followed only if the follower owns it or the directory's owner does. Root is held to it too: it is the usual victim. (Linux: `fs.protected_symlinks = 1`.)

## Try it

```
seclab verify protected_symlinks
```
