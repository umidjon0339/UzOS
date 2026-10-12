void _start() {
    volatile char *video = (volatile char*)0xB8000;
    const char *str = "Hello from dynamically loaded ELF binary!";
    int i = 0;
    
    // Write to the second line of VGA text mode
    while (str[i]) {
        video[160 + i * 2] = str[i];
        video[160 + i * 2 + 1] = 0x0A; // Bright Green on Black
        i++;
    }
    
    // Infinite loop since we don't have a syscall for exit yet
    while(1) {
        asm volatile("hlt");
    }
}
