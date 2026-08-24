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

.PHONY: all build clean run debug

all: build

build: build/myos.bin

run: build
	qemu-system-x86_64 -kernel build/myos.bin

debug: build
	qemu-system-x86_64 -kernel build/myos.bin -S -s

build/myos.bin: build/myos.elf
	$(OBJCOPY) -O elf32-i386 $< $@

build/myos.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

build/boot.o: boot/boot.S
	@mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

build/boot64.o: boot/boot64.S
	@mkdir -p build
	$(ASM) $(ASMFLAGS) $< -o $@

build/%.o: kernel/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
