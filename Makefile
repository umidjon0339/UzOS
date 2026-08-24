CC = gcc
LD = ld
ASM = nasm
OBJCOPY = objcopy

# -mno-red-zone is required for kernel space to avoid red-zone clobbering on interrupts
# -mno-sse -mno-sse2 -mno-mmx prevents GCC from emitting vector instructions before FPU/SSE is enabled
CFLAGS = -ffreestanding -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -m64 -fno-stack-protector -fno-pic -fno-pie -O2 -Wall -Wextra -Iinclude
LDFLAGS = -n -T linker.ld -nostdlib -z max-page-size=0x1000 -m elf_x86_64
ASMFLAGS = -f elf64

OBJS = build/boot.o build/boot64.o build/isr.o build/terminal.o build/keyboard.o build/idt.o build/string.o build/shell.o build/kernel.o

.PHONY: all build clean run run-iso iso debug

all: build

build: build/myos.bin

run: build
	qemu-system-x86_64 -kernel build/myos.bin

run-iso: iso
	qemu-system-x86_64 -cdrom build/myos.iso -boot d

iso: build/myos.iso

# Generates a bootable ISO using GRUB (grub-mkrescue)
# Uses Multiboot 1 to load the kernel in VGA text mode
build/myos.iso: build/myos.bin
	@mkdir -p build/isodir/boot/grub
	@cp build/myos.bin build/isodir/boot/myos.bin
	@echo 'insmod all_video' > build/isodir/boot/grub/grub.cfg
	@echo 'terminal_input console' >> build/isodir/boot/grub/grub.cfg
	@echo 'terminal_output console' >> build/isodir/boot/grub/grub.cfg
	@echo 'set timeout=10' >> build/isodir/boot/grub/grub.cfg
	@echo 'set default=0' >> build/isodir/boot/grub/grub.cfg
	@echo 'menuentry "Try MyOS v0.1 (Live Mode)" {' >> build/isodir/boot/grub/grub.cfg
	@echo '  set gfxpayload=1024x768x32' >> build/isodir/boot/grub/grub.cfg
	@echo '  multiboot /boot/myos.bin' >> build/isodir/boot/grub/grub.cfg
	@echo '  boot' >> build/isodir/boot/grub/grub.cfg
	@echo '}' >> build/isodir/boot/grub/grub.cfg
	@echo 'menuentry "Install MyOS v0.1 to Disk" {' >> build/isodir/boot/grub/grub.cfg
	@echo '  set gfxpayload=1024x768x32' >> build/isodir/boot/grub/grub.cfg
	@echo '  multiboot /boot/myos.bin' >> build/isodir/boot/grub/grub.cfg
	@echo '  boot' >> build/isodir/boot/grub/grub.cfg
	@echo '}' >> build/isodir/boot/grub/grub.cfg
	grub-mkrescue -o build/myos.iso build/isodir

debug: build
	qemu-system-x86_64 -kernel build/myos.bin -S -s

build/myos.bin: build/myos.elf
	$(OBJCOPY) -O elf32-i386 $< $@

build/myos.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

build/boot.o: boot/boot.asm
	@mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

build/boot64.o: boot/boot64.asm
	@mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

build/isr.o: boot/isr.asm
	@mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

build/%.o: kernel/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
