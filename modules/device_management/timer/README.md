# Programmable Interval Timer (PIT) Driver

## Educational Concepts: The System Clock and IRQ0
The Intel 8253/8254 PIT is a hardware timer that continually triggers an interrupt (IRQ0) at a specified frequency. 
When the system boots, it ticks at 18.222 Hz by default. We can change this by writing to its control ports (0x43) and data ports (0x40).

The timer is essential for operating systems because it provides the "heartbeat" required to:
- Keep track of system uptime.
- Provide a `sleep()` function so the CPU isn't spinning endlessly.
- Trigger the **Scheduler** for multitasking. When the timer interrupts a program, the OS can pause it and resume a different program, giving the illusion of multiple programs running simultaneously.
