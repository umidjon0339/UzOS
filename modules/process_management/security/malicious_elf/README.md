# Attack: malicious_elf

| | |
|---|---|
| **Stopped by** | [`elf_validate`](../../defence/elf_validate/) |
| **Breaks** | trust: input from outside is checked before use |
| **Severity** | critical |

## What it does

`exec <file>` loads an ELF program. The loader copies bytes to the
addresses the **file** names and jumps to the entry point the **file**
names. Whoever writes the file controls those numbers:

| Crafted value | Effect without checks |
|---|---|
| segment at `0x00100000` | overwrites the kernel's own code |
| segment `0xFFFFF000` + `0x2000` bytes | wraps past 4 GiB into low memory |
| `p_filesz > p_memsz` | copies past the end of the segment |
| `e_phnum = 200` | overflows the loader's 16-entry array on the stack |
| `e_entry = 0x00100000` | jumps straight into kernel code |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the tricks |
| `live` | builds each crafted header in memory and runs it through the same checks `elf_load_and_run()` uses; then checks that a normal program still passes |

Nothing is ever loaded.

## Uses

`def_elf_check_header()` / `def_elf_check_segments()` from
defence/elf_validate, `Elf32_Ehdr` / `Elf32_Phdr` from core/elf.

## Try it

```
seclab attack malicious_elf live
exec hello                       # a real program still runs
```
