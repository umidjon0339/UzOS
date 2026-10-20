# arch/x86 — CPU setup for 32-bit x86

Everything here is specific to the x86 CPU. Porting UzOS to another CPU
would mean replacing this folder. Each feature has its own folder that
holds all of its code: the header, the C file and any assembly.

```
arch/x86/
├── io/         io.h                 in/out instructions for device ports
├── cpu/        cpu.h                interrupts off/on, read ESP, CR0/CR4, rdtsc
├── cpuid/      cpuid.h cpuid.c      run cpuid: vendor, name, any leaf (cpu_management decodes)
├── gdt/        gdt.h gdt.c gdt_load.s   memory segments (kernel / user)
├── tss/        tss.h tss.c          kernel stack for ring 3 -> ring 0
├── idt/        idt.h idt.c          table: interrupt vector -> handler
├── isr/        isr.h isr.c isr_stubs.s  interrupt entry + C dispatch
└── usermode/   usermode.h usermode.s    jump from ring 0 to ring 3
```

## Order of setup

`kernel_main()` calls these right after the screen is ready:

1. **`init_gdt()`** loads the segments and the TSS.
   The CPU can't handle interrupts before this.
2. **`init_idt()`** points all 256 vectors at their stubs.
3. `pic_remap()` (device driver) moves IRQ 0-15 to vectors 32-47.
4. Drivers call **`register_interrupt_handler()`** for their vector.
5. `sti` turns interrupts on.

## GDT: segments

The CPU checks every memory access against a segment. UzOS uses a
**flat model**: all segments start at 0 and cover 4 GiB, so segments
only decide the **ring** (0 = kernel, 3 = user). Paging does the real
memory protection.

| Slot | Selector | Segment     |
|------|----------|-------------|
| 0    | 0x00     | null (required) |
| 1    | 0x08     | kernel code |
| 2    | 0x10     | kernel data |
| 3    | 0x1B     | user code (0x18 \| ring 3) |
| 4    | 0x23     | user data (0x20 \| ring 3) |
| 5    | 0x28     | TSS |

## TSS: the kernel stack for ring changes

When an interrupt hits while user code runs, the CPU switches to the
stack stored in the TSS (`ss0:esp0`). The scheduler updates it on every
task switch with `set_kernel_stack()`. The I/O bitmap offset points past
the TSS, so user code can't use port I/O.

## IDT and ISRs: interrupts

```
device / CPU fault / int 0x80
        │
        ▼
IDT[vector] ──► isrN / irqN stub    (isr_stubs.s: push error code + vector)
                      │
                      ▼
                 isr_common          (save registers, kernel segments, cld)
                      │
                      ▼
                 isr_handler(regs)   (isr.c)
                   ├─ 32-47: send EOI to the PIC, run the driver's handler
                   ├─ registered handler: run it (page fault, syscall, ...)
                   └─ unhandled exception: print registers, halt
```

| Vectors | Meaning |
|---------|---------|
| 0-31    | CPU exceptions (0 divide error, 13 general protection, 14 page fault) |
| 32-47   | hardware IRQs (32 timer, 33 keyboard, 44 mouse) |
| 128     | system call: `int 0x80`, the only gate ring 3 may call |

**Who handles exceptions?** This folder only routes them.
[cpu_management/core/faults](../../modules/cpu_management/core/faults/)
registers a handler for every exception (page faults stay in
memory_management/core/vmm). A fault from ring 3 kills that process. A
fault from ring 0 calls `isr_panic()`, which prints the registers and
halts.

## Control registers and the cycle counter

`cpu.h` has the raw instructions for these. What to switch on is
decided elsewhere:

| Register / bit | Set by | Meaning |
|---|---|---|
| CR0.PE | GRUB | protected mode |
| CR0.PG | memory_management/core/vmm | paging on |
| CR4.TSD | cpu_management defence `tsc_restrict` | `rdtsc` only in ring 0 |
| `rdtsc` | cpu_management/core/tsc | cycles since power on |

`cpuinfo` shows them all.

## User mode

There is no instruction that simply means "go to ring 3". `switch_to_user_mode()`
pushes the frame an interrupt would have pushed (SS, ESP, EFLAGS, CS, EIP)
with ring 3 selectors, then runs `iret`.
