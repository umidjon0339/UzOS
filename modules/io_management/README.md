# I/O Management

I/O Management provides the common descriptor interface between processes,
the terminal devices, and the VFS. Device-specific input/output stays in
Device Management; files and directories stay in Filesystem Management.

```text
io_management/
├── core/                 # Descriptor and transfer validation
├── commands/             # I/O inspection commands
├── system_calls/         # Open, close, read, and write
└── security/
    ├── attack/           # Safe descriptor and buffer probes
    └── defence/          # Checks used by I/O operations
```

## Descriptors

| Descriptor | Stream |
|---|---|
| `0` | Standard input: queued keyboard keys, non-blocking |
| `1` | Standard output: VGA text terminal |
| `2` | Standard error: VGA text terminal |
| `3`–`MAX_FDS - 1` | Open VFS files |

Standard descriptors are reserved and cannot be closed. File handles are
allocated from descriptor 3. Transfers validate their buffer range and are
limited to 16 KiB.

## Command

`ioinfo` displays the current process's standard streams and open file
descriptors.

```text
ioinfo
```

## Security pairs

| Attack | Defence | Purpose |
|---|---|---|
| `invalid_descriptor` | `descriptor_bounds` | Reject out-of-range, closed, and non-file descriptors before table access. |
| `oversized_transfer` | `transfer_bounds` | Reject null, wrapped, or over-limit I/O buffers. |

```text
seclab attack invalid_descriptor live
seclab attack oversized_transfer live
seclab verify descriptor_bounds
seclab verify transfer_bounds
```

These routines are currently callable kernel interfaces, matching the
existing filesystem operations. Ring-3 `int 0x80` dispatch is not yet
implemented by the generic syscall handler.
