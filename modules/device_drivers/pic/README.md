# PIC (Programmable Interrupt Controller)

## Educational Concepts: Hardware Interrupt Routing and I/O Ports
The Intel 8259 PIC is a chip that manages hardware interrupts. When you press a key on your keyboard, it doesn't talk directly to the CPU. Instead, it sends an electrical signal (an IRQ, or Interrupt Request) to the PIC. The PIC decides how important the signal is and forwards it to the CPU along with a designated IDT vector number.

### Remapping
By default, the master PIC uses vectors 0-7, and the slave PIC uses vectors 8-15. This is a huge problem because the CPU reserves vectors 0-31 for exceptions (like Page Faults)! If we don't remap the PIC, hardware interrupts will overlap with software exceptions, causing total chaos.

To fix this, we send "Initialization Command Words" (ICWs) to the PIC via CPU I/O ports (`0x20` and `0xA0`). We reprogram the Master PIC to start at vector 32 (`0x20`) and the Slave PIC to start at vector 40 (`0x28`).
