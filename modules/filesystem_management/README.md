# Filesystem Management

## Security

```text
security/
├── attack/    # Safe path and file-range attack demonstrations
└── defence/   # Filesystem policies wired into VFS and RAM-disk operations
```

| Attack | Defence | Protects |
|---|---|---|
| `path_traversal` | `path_component` | Bound names to a single supported path component. |
| `file_offset_wrap` | `file_range` | Prevent file offsets and lengths from exceeding safe limits. |

Try the pairs using the system security lab:

```text
seclab attack path_traversal live
seclab attack file_offset_wrap live
seclab verify path_component
seclab verify file_range
```

The filesystem supports root-level or current-directory names, not
nested path traversal. `cd ..` remains available for moving to a parent
directory; file creation and access paths are validated separately.
