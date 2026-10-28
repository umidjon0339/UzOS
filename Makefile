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
# holds a .h goes on the include path. The project root (.) is on it too,
# so a file may also name a header by its full path:
#     #include "modules/cpu_management/core/tsc/tsc.h"
INC_DIRS := . $(sort $(dir $(shell find $(SRC_DIRS) -name '*.h')))

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

# The kernel is linked twice, so that it can name its own functions
# (`perf report` shows "idle_task" instead of "0x00104a2c"):
#   1. link without names                         build/kernel.nosyms
#   2. list its functions (nm) as a C table       build/ksyms.c
#   3. link again, with that table added          kernel.bin
# The table is only constants (.rodata comes after .text in linker.ld),
# so no function moves between link 1 and link 3: the addresses in the
# table are right. The table belongs to `perf report`:
# see modules/cpu_management/commands/perf_report/ksyms.c.
KSYMS_GEN := modules/cpu_management/commands/perf_report/ksyms_gen.sh

$(KERNEL): $(OBJ) boot/linker.ld $(KSYMS_GEN)
	@echo "  LD   $(BUILD)/kernel.nosyms"
	@$(CC) $(LDFLAGS) $(OBJ) -o $(BUILD)/kernel.nosyms -lgcc
	@echo "  GEN  $(BUILD)/ksyms.c"
	@sh $(KSYMS_GEN) $(BUILD)/kernel.nosyms > $(BUILD)/ksyms.c
	@$(CC) $(CFLAGS) -c $(BUILD)/ksyms.c -o $(BUILD)/ksyms.o
	@echo "  LD   $@"
	@$(CC) $(LDFLAGS) $(OBJ) $(BUILD)/ksyms.o -o $@ -lgcc

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
