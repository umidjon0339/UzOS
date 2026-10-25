# Defence: elf_validate

| | |
|---|---|
| **Stops** | [`malicious_elf`](../../attack/malicious_elf/) |
| **Protects** | trust: input from a file is checked before use |
| **Called from** | `elf_load_and_run()` in [core/elf](../../../core/elf/elf.c), twice |

## The rules

**Header** (`def_elf_check_header`), before any other field is used:

- magic `\x7F ELF`, 32-bit, little-endian, executable, i386
- 1 to 16 program headers, each exactly `sizeof(Elf32_Phdr)`
- entry point inside user space

**Segments** (`def_elf_check_segments`), for every `PT_LOAD`:

- `p_filesz <= p_memsz`
- `[p_vaddr, p_vaddr + p_memsz)` lies inside user space and does not wrap

## Why

The loader copies bytes to the addresses the file asks for. Without
checks, a crafted file could:

| Trick | Result |
|---|---|
| `p_vaddr = 0x00100000` | overwrite the kernel's own code |
| `p_vaddr + p_memsz` past 4 GiB | wrap around to low memory |
| `e_phnum = 200` | overflow the loader's 16-entry array on the stack |
| `e_entry` in the kernel | jump straight into kernel code |

## Uses

- `USER_SPACE_START` / `USER_SPACE_END` from memory_management/core/layout
- `Elf32_Ehdr` / `Elf32_Phdr` from core/elf
- `audit_log()` from security_management (core/audit)

## Try it

```
exec hello                        # a good file still loads
seclab attack malicious_elf live
seclab verify elf_validate
```
