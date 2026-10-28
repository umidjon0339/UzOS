#ifndef PERF_H
#define PERF_H

/*
 * perf.h - Sampling: where does the CPU spend its time?
 *
 * `perf record` asks: WHERE is the time spent? On every timer tick the
 * kernel notes who was running and at which instruction (EIP). After
 * thousands of samples, the functions that show up most are the ones
 * that use the CPU (`perf report`).
 *
 * This lives in core because the TIMER calls it (through
 * core/accounting) on every tick, whether a command is running or not.
 * The counters of perf_event_open() are in that syscall's own folder.
 */

#include <stdint.h>
#include "arch/x86/isr/isr.h"                                    // registers_t
#include "modules/process_management/core/process/process.h"    // process_t

#define PERF_MAX_SAMPLES 2048   // samples one `perf record` keeps (20 s at 100 Hz)

// One sample: who ran, where, in which ring.
typedef struct {
    uint32_t eip;
    uint32_t pid;
    uint8_t  user;              // 1 = ring 3
    char     name[11];          // the process name, cut to 10
} perf_sample_t;

void perf_record_start(void);           // forget old samples, start taking new ones
void perf_record_stop(void);
int  perf_recording(void);

// Called on every timer tick (core/accounting) with the interrupted
// registers and process. Does nothing unless recording.
void perf_sample(const registers_t *regs, const process_t *p);

int perf_sample_count(void);
const perf_sample_t *perf_sample_get(int index);
uint32_t perf_samples_lost(void);       // ticks after the buffer was full

/* ---------- The perf.data file ----------
 * `perf record` writes the samples to a file, `perf report` reads it
 * back (any time later, even after other recordings):
 *
 *   perf_file_header_t     magic "PERF", how many samples, ...
 *   perf_sample_t[samples] the samples, oldest first
 */
#define PERF_FILE_MAGIC 0x46524550      // "PERF" read as a little-endian number

typedef struct {
    uint32_t magic;
    uint32_t samples;       // how many perf_sample_t follow
    uint32_t lost;          // samples that did not fit
    uint32_t hz;            // samples per second (the timer rate)
} perf_file_header_t;

#endif
