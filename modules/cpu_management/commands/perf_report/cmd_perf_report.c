/*
 * cmd_perf_report.c - `perf report`: which functions used the CPU?
 *
 *   # Samples: 412 of event 'cpu-clock' (timer tick, 100 Hz)
 *   # Overhead  Command       Shared Object  Symbol
 *       91.26%  idle          [kernel]       [k] power_idle
 *        4.13%  kernel_main   [kernel]       [k] vga_put_char
 *        1.21%  spin          spin           [.] 0x40000004
 *
 *   perf report                  read ./perf.data
 *   perf report -i run1.data     another file
 *   perf report --sort comm      by process only
 *
 * Each sample is an EIP. For kernel code (ring 0, "[k]") the kernel's
 * own symbol table (ksyms.c, in this folder) turns it into a function name; ring 3
 * programs ("[.]") have no symbols in UzOS, so their address is shown.
 * Overhead = the share of all samples that fell into that function:
 * where the time went.
 *
 * Uses: perf_file_header_t / perf_sample_t (core/perf), ksyms_lookup
 *       (ksyms.c, this folder), sys_open / sys_read / sys_close (io_management),
 *       def_safe_text (ui defence safe_text), ui_* (ui), arg_next /
 *       format / string (lib).
 */

#include "modules/cpu_management/commands/perf_report/cmd_perf_report.h"   // this command
#include "modules/cpu_management/core/perf/perf.h"                         // the file format
#include "modules/cpu_management/commands/perf_report/ksyms.h"                       // ksyms_lookup
#include "modules/io_management/syscalls/open/sys_open.h"                  // sys_open, O_READ
#include "modules/io_management/syscalls/read/sys_read.h"                  // sys_read
#include "modules/io_management/syscalls/close/sys_close.h"                // sys_close
#include "modules/ui/security/defence/safe_text/def_safe_text.h"           // def_safe_text
#include "modules/ui/core/style/ui.h"                                      // ui_*
#include "lib/args/args.h"                                                 // arg_next
#include "lib/format/format.h"                                             // format
#include "lib/string/string.h"                                             // strcmp, strncpy

#define MAX_ROWS   64           // different (command, symbol) pairs kept
#define SHOW_ROWS  18           // rows printed

typedef struct {
    char     comm[11];
    char     symbol[28];
    uint8_t  user;
    uint32_t count;
} report_row_t;

static report_row_t rows[MAX_ROWS];
static int row_count;
static uint32_t other;          // samples that did not fit in rows[]

static void add(const perf_sample_t *s, int by_comm) {
    char symbol[28] = "";
    if (!by_comm) {
        uint32_t offset;
        const char *name = s->user ? 0 : ksyms_lookup(s->eip, &offset);
        if (name)
            format(symbol, sizeof(symbol), "%s", name);
        else
            format(symbol, sizeof(symbol), "0x%08x", s->eip);
    }

    for (int i = 0; i < row_count; i++) {
        if (rows[i].user == s->user && strcmp(rows[i].comm, s->name) == 0 &&
            strcmp(rows[i].symbol, symbol) == 0) {
            rows[i].count++;
            return;
        }
    }
    if (row_count >= MAX_ROWS) {
        other++;
        return;
    }
    report_row_t *r = &rows[row_count++];
    strncpy(r->comm, s->name, sizeof(r->comm) - 1);
    r->comm[sizeof(r->comm) - 1] = '\0';
    strncpy(r->symbol, symbol, sizeof(r->symbol) - 1);
    r->symbol[sizeof(r->symbol) - 1] = '\0';
    r->user = s->user;
    r->count = 1;
}

static void sort_rows(void) {
    for (int i = 1; i < row_count; i++) {
        report_row_t r = rows[i];
        int j = i - 1;
        while (j >= 0 && rows[j].count < r.count) {
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = r;
    }
}

static void print_rows(uint32_t total, int by_comm) {
    ui_color(UI_TITLE);
    if (by_comm)
        vga_print_str("# Overhead  Command\n# ........  ...........\n");
    else
        vga_print_str("# Overhead  Command      Shared Object  Symbol\n"
                      "# ........  ...........  .............  ..........................\n");
    ui_reset();

    for (int i = 0; i < row_count && i < SHOW_ROWS; i++) {
        report_row_t *r = &rows[i];
        uint32_t h = (uint32_t)((uint64_t)r->count * 10000 / total);
        char comm[12], object[12];
        def_safe_text(r->comm, comm, 11);
        ui_color(h >= 2000 ? UI_BAD : h >= 500 ? UI_WARNING : ui_text_fg());
        ui_printf("  %3u.%02u%%  ", h / 100, h % 100);
        ui_reset();
        if (by_comm) {
            ui_printf("%s\n", comm);
            continue;
        }
        format(object, sizeof(object), "%s", r->user ? comm : "[kernel]");
        ui_printf("%-11s  %-13s  [%c] %s\n", comm, object, r->user ? '.' : 'k', r->symbol);
    }
    if (row_count > SHOW_ROWS || other)
        ui_hint("(smaller entries not shown)");
}

void cmd_perf_report(const char *args) {
    char word[64], file[64] = "perf.data";
    int by_comm = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-i") == 0) {
            args = arg_next(args, file, sizeof(file));
        } else if (strcmp(word, "--sort") == 0) {
            args = arg_next(args, word, sizeof(word));
            by_comm = strcmp(word, "comm") == 0;
        } else if (strcmp(word, "--sort=comm") == 0) {
            by_comm = 1;
        } else {
            ui_usage("perf report [-i file] [--sort comm|sym]");
            return;
        }
    }

    int fd = sys_open(file, O_READ);
    perf_file_header_t header;
    if (fd < 0 || sys_read(fd, &header, sizeof(header)) != (int)sizeof(header) ||
        header.magic != PERF_FILE_MAGIC) {
        if (fd >= 0)
            sys_close(fd);
        ui_error("perf", "no perf data (run `perf record command` first)");
        return;
    }

    row_count = 0;
    other = 0;
    perf_sample_t s;
    uint32_t total = 0;
    while (total < header.samples && sys_read(fd, &s, sizeof(s)) == (int)sizeof(s)) {
        s.name[sizeof(s.name) - 1] = '\0';          // a file can contain anything
        add(&s, by_comm);
        total++;
    }
    sys_close(fd);
    if (!total) {
        ui_info("perf", "the file has no samples");
        return;
    }

    sort_rows();
    ui_color(UI_DIM);
    ui_printf("# Samples: %u of event 'cpu-clock' (timer tick, %u Hz)", total, header.hz);
    if (header.lost)
        ui_printf(", %u lost", header.lost);
    vga_print_str("\n");
    ui_reset();
    print_rows(total, by_comm);
}
