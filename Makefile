CC = gcc
LD = ld
ASM = nasm
OBJCOPY = objcopy

# -mno-red-zone is required for kernel space to avoid red-zone clobbering on interrupts
# -mno-sse -mno-sse2 -mno-mmx prevents GCC from emitting vector instructions before FPU/SSE is enabled
CFLAGS = -ffreestanding -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -m64 -fno-stack-protector -fno-pic -fno-pie -O2 -Wall -Wextra -Iinclude
LDFLAGS = -n -T linker.ld -nostdlib -z max-page-size=0x1000 -m elf_x86_64
ASMFLAGS = -f elf64

OBJS = build/boot.o build/boot64.o build/terminal.o build/kernel.o

.PHONY: all build clean run run-iso iso debug

all: build

build: build/myos.bin

run: build
	qemu-system-x86_64 -kernel build/myos.bin

run-iso: iso
	qemu-system-x86_64 -cdrom build/myos.iso

iso: build/myos.iso

# Generates a bootable ISO using GRUB (grub-mkrescue)
build/myos.iso: build/myos.elf
	@mkdir -p build/isodir/boot/grub
	@cp build/myos.elf build/isodir/boot/myos.elf
	@echo 'set timeout=3' > build/isodir/boot/grub/grub.cfg
	@echo 'set default=0' >> build/isodir/boot/grub/grub.cfg
	@echo 'menuentry "MyOS" {' >> build/isodir/boot/grub/grub.cfg
	@echo '  multiboot /boot/myos.elf' >> build/isodir/boot/grub/grub.cfg
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

build/%.o: kernel/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
