/*
 * cmd_perf_record.c - `perf record command`: where does the CPU spend its time?
 *
 *   perf record ls                   sample while ls runs -> ./perf.data
 *   perf record -o run1.data spawn   write to another file
 *   perf report                      look at the result (perf_report/)
 *
 * While the command runs, every timer tick (100 a second) the kernel
 * notes which process was on the CPU, in which ring, and at which
 * instruction (EIP): core/perf. Like Linux's `perf record -a`, the
 * samples cover the whole system, so you also see what ran BESIDES your
 * command (the idle task, statusd...). Afterwards the samples are
 * written to a file with the normal file syscalls.
 *
 *   [ perf record: Woken up 1 times to write data ]
 *   [ perf record: Captured and wrote 0.008 MB perf.data (412 samples) ]
 *
 * Uses: perf_record_* / perf_sample_* (core/perf), perf_record_run_child
 *       (run_child.c, this folder), sys_open / sys_write / sys_close (io_management),
 *       timer_frequency (device_management), ui_* (ui), arg_next /
 *       strcmp (lib).
 */

#include "modules/cpu_management/commands/perf_record/cmd_perf_record.h"   // this command
#include "modules/cpu_management/core/perf/perf.h"                         // perf_record_*, perf_file_header_t
#include "modules/cpu_management/commands/perf_record/run_child.h"      // perf_record_run_child (this folder)
#include "modules/io_management/syscalls/open/sys_open.h"                  // sys_open, O_*
#include "modules/io_management/syscalls/write/sys_write.h"                // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"                // sys_close
#include "modules/device_management/timer/timer.h"                         // timer_frequency
#include "modules/ui/core/style/ui.h"                                      // ui_*
#include "lib/args/args.h"                                                 // arg_next
#include "lib/string/string.h"                                             // strcmp

#define DEFAULT_FILE "perf.data"

static int save(const char *file) {
    int fd = sys_open(file, O_WRITE | O_CREATE | O_TRUNC);
    if (fd < 0)
        return 0;

    perf_file_header_t header = { PERF_FILE_MAGIC, perf_sample_count(),
                                  perf_samples_lost(), timer_frequency() };
    int ok = sys_write(fd, &header, sizeof(header)) == (int)sizeof(header);
    for (int i = 0; ok && i < perf_sample_count(); i++)
        ok = sys_write(fd, perf_sample_get(i), sizeof(perf_sample_t)) == (int)sizeof(perf_sample_t);
    sys_close(fd);
    return ok;
}

void cmd_perf_record(const char *args) {
    char word[64], file[64] = DEFAULT_FILE;
    const char *rest = arg_next(args, word, sizeof(word));

    if (strcmp(word, "-o") == 0) {
        rest = arg_next(rest, file, sizeof(file));
    } else {
        rest = args;
    }
    while (*rest == ' ')
        rest++;
    if (!*rest || !file[0]) {
        ui_usage("perf record [-o file] command");
        return;
    }

    perf_record_start();
    int pid = perf_record_run_child(rest);
    perf_record_stop();
    if (pid < 0) {
        ui_error("perf", "cannot run that command");
        return;
    }

    if (!save(file)) {
        ui_error("perf", "could not write the data file");
        return;
    }
    uint32_t bytes = sizeof(perf_file_header_t) + perf_sample_count() * sizeof(perf_sample_t);
    ui_printf("[ perf record: Woken up 1 times to write data ]\n");
    ui_printf("[ perf record: Captured and wrote 0.%03u MB %s (%d samples) ]\n",
              bytes / 1000 % 1000, file, perf_sample_count());
    if (perf_samples_lost())
        ui_warn("perf", "the sample buffer was full: the end of the run is missing");
}
