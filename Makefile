AS = nasm
CC = gcc
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -Iinclude -fno-pie -fno-stack-protector -mno-sse -mno-mmx -mno-sse2
LDFLAGS = -m32 -nostdlib -T linker.ld -no-pie
ASFLAGS = -f elf32

# Find all C files in modules/**/core, system_calls, commands, security
C_SOURCES = $(wildcard modules/*/core/*.c) \
            $(wildcard modules/*/system_calls/*.c) \
            $(wildcard modules/*/commands/*.c) \
            $(wildcard modules/*/security/*.c) \
            $(wildcard modules/*/*/core/*.c) \
            $(wildcard modules/*/*/system_calls/*.c) \
            $(wildcard modules/*/*/commands/*.c) \
            $(wildcard modules/*/*/security/*.c) \
            $(wildcard modules/*/commands/*/*.c) \
            $(wildcard modules/*/system_calls/*/*.c) \
            $(wildcard modules/*/security/*/*.c) \
            $(wildcard modules/*/*/commands/*/*.c) \
            $(wildcard modules/*/*/system_calls/*/*.c) \
            $(wildcard modules/*/*/security/*/*.c)


# We will also include arch/x86/ C files
C_SOURCES += $(wildcard arch/x86/*.c)

# Find all assembly files
S_SOURCES = boot/boot.s $(wildcard arch/x86/*.s) \
            $(wildcard modules/*/core/*.s) \
            $(wildcard modules/*/*/core/*.s)

# Object files
OBJ = $(S_SOURCES:.s=.o) $(C_SOURCES:.c=.o)

KERNEL = kernel.bin

all: $(KERNEL)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ -lgcc

run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

clean:
	rm -f $(OBJ) $(KERNEL)

.PHONY: all run clean
