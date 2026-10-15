# PS/2 Keyboard Driver

## Educational Concepts: Scancodes and IRQ1
When you press a key on a standard PS/2 keyboard, the keyboard controller sends an interrupt (IRQ1) to the PIC. The CPU stops what it's doing and jumps to our IRQ1 handler.

Inside the handler, we use the `inb` instruction to read a byte from the I/O port `0x60`. This byte is a **Scancode**.
- Pressing a key sends a "Make Code" (e.g., `0x1E` for 'A').
- Releasing a key sends a "Break Code", which is usually the Make Code `+ 0x80` (e.g., `0x9E` for releasing 'A').

To convert this raw hardware data into text, the OS uses an array mapping scancodes to standard ASCII characters.
