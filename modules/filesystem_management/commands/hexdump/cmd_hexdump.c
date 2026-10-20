/*
 * cmd_hexdump.c - `hexdump <path> [bytes]`: show a file byte by byte.
 *
 *   00000000  7F 45 4C 46 01 01 01 00 ...  .ELF....
 *   offset    16 bytes in hex                same bytes as text
 *
 * Shows the first 256 bytes unless you ask for more.
 *
 * Uses: sys_open / sys_read / sys_close (io_management), arg_next (lib/args),
 *       atoi (lib/stdlib), ui_* (ui module).
 */

#include "fs_commands.h"
#include "sys_open.h"
#include "sys_read.h"
#include "sys_close.h"
#include "args.h"
#include "stdlib.h"
#include "vfs.h"
#include "ui.h"

#define ROW      16
#define DEFAULT  256

static const char hex[] = "0123456789ABCDEF";

static void print_row(uint32_t offset, const uint8_t *bytes, int n) {
    for (int shift = 28; shift >= 0; shift -= 4)        // 8-digit offset
        vga_put_char(hex[(offset >> shift) & 0xF]);
    vga_print_str("  ");

    for (int i = 0; i < ROW; i++) {
        if (i < n) {
            vga_put_char(hex[bytes[i] >> 4]);
            vga_put_char(hex[bytes[i] & 0xF]);
            vga_put_char(' ');
        } else {
            vga_print_str("   ");                       // pad a short last row
        }
    }
    vga_put_char(' ');
    for (int i = 0; i < n; i++)
        vga_put_char(bytes[i] >= 32 && bytes[i] < 127 ? bytes[i] : '.');
    vga_put_char('\n');
}

void cmd_hexdump(const char *args) {
    char path[FS_PATH_MAX], count_text[16];
    arg_next(arg_next(args, path, sizeof(path)), count_text, sizeof(count_text));

    int fd = path[0] ? sys_open(path, O_READ) : -1;
    if (fd < 0) {
        ui_usage("hexdump <path> [bytes]   (file must exist)");
        return;
    }
    int limit = count_text[0] ? atoi(count_text) : DEFAULT;

    uint8_t row[ROW];
    int n;
    uint32_t offset = 0;
    while ((int)offset < limit && (n = sys_read(fd, row, ROW)) > 0) {
        print_row(offset, row, n);
        offset += n;
    }
    sys_close(fd);
}
