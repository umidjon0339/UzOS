# =============================================================================
# UzOS build
#
#   make         build kernel.bin
#   make run     boot it in QEMU
#   make check   check that GRUB/QEMU can find the multiboot header
#   make clean   delete everything the build made
#
# Every .c and .s file under SRC_DIRS is compiled. Object files go to
# build/, mirroring the source tree, so source folders hold only code.
# =============================================================================

CC     := gcc
AS     := nasm
BUILD  := build
KERNEL := kernel.bin

SRC_DIRS := boot arch lib modules

# --- Sources -----------------------------------------------------------------
C_SOURCES := $(shell find $(SRC_DIRS) -name '*.c')
S_SOURCES := $(shell find $(SRC_DIRS) -name '*.s')

OBJ := $(patsubst %.c,$(BUILD)/%.o,$(C_SOURCES)) \
       $(patsubst %.s,$(BUILD)/%.o,$(S_SOURCES))
DEP := $(OBJ:.o=.d)

# Each feature keeps its header in its own folder, so every folder that
# holds a .h goes on the include path.
INC_DIRS := $(sort $(dir $(shell find $(SRC_DIRS) -name '*.h')))

# --- Flags -------------------------------------------------------------------
# -m32 -ffreestanding     32-bit kernel code, no hosted C library
# -fno-pie                code runs at the fixed address in boot/linker.ld
# -fno-stack-protector    no libc to provide __stack_chk_fail
# -mno-sse -mno-mmx ...   the kernel never saves SSE/MMX registers
# -MMD -MP                write .d files so header edits trigger rebuilds
CFLAGS := -m32 -ffreestanding -O2 -Wall -Wextra \
          -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
          -mno-sse -mno-sse2 -mno-mmx \
          -MMD -MP $(addprefix -I,$(INC_DIRS))

ASFLAGS := -f elf32

LDFLAGS := -m32 -nostdlib -no-pie -T boot/linker.ld -Wl,--build-id=none

# --- Rules -------------------------------------------------------------------
all: $(KERNEL)

$(KERNEL): $(OBJ) boot/linker.ld
	@echo "  LD   $@"
	@$(CC) $(LDFLAGS) $(OBJ) -o $@ -lgcc

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "  AS   $<"
	@$(AS) $(ASFLAGS) $< -o $@

# -rtc base=localtime: the clock chip shows your local time, not UTC.
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -rtc base=localtime

check: $(KERNEL)
	@grub-file --is-x86-multiboot $(KERNEL) && echo "$(KERNEL): valid multiboot kernel"

clean:
	rm -rf $(BUILD) $(KERNEL)

.PHONY: all run check clean

-include $(DEP)
