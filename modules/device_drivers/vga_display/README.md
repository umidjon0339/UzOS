# VGA Display Module

## Educational Concepts: Memory-Mapped I/O and VGA Text Mode

This module introduces the concept of Memory-Mapped I/O (MMIO). In x86 architecture, the CPU can communicate with external hardware devices (like the graphics card) by reading from and writing to specific memory addresses.

For the standard VGA text mode, the video memory is mapped starting at physical address `0xB8000`. 
This buffer is generally structured as a 2D array of 80 columns by 25 rows.

Each character displayed on the screen takes exactly 2 bytes in memory:
1. **Character Byte**: The ASCII value of the character to display.
2. **Attribute Byte**: Determines the foreground and background colors of the character.

### Attribute Byte Layout
The attribute byte is split into two nibbles (4 bits each):
- Lower 4 bits (0-3): Foreground color (0-15)
- Upper 4 bits (4-7): Background color (0-15)

By writing a 16-bit value (the combination of character and attribute) to `0xB8000 + offset`, the VGA hardware automatically updates the physical screen. This driver implements the logic to keep track of the cursor position, handle newlines, scrolling, and printing basic strings and numbers to the screen.
