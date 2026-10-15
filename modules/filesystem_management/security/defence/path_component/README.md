# Defence: path component

**Counters:** [`path_traversal`](../../attack/path_traversal/README.md)

The shared path policy accepts bounded single-component names (with an
optional leading slash), and rejects empty names, `.`/`..`, nested paths,
and names too long for a VFS node. Open, create, delete, and directory
change operations use the policy. `cd ..` remains an explicit safe
parent-directory operation.

```text
seclab verify path_component
seclab attack path_traversal live
```
