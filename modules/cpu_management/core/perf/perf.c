/*
 * perf.c - The sample buffer of `perf record`.
 *
 * Uses: strncpy (lib/string).
 */

#include "modules/cpu_management/core/perf/perf.h"     // this part
#include "lib/string/string.h"                         // strncpy

static perf_sample_t samples[PERF_MAX_SAMPLES];
static int sample_count;
static uint32_t samples_lost;
static int recording;

void perf_record_start(void) {
    sample_count = 0;
    samples_lost = 0;
    recording = 1;
}

void perf_record_stop(void) {
    recording = 0;
}

int perf_recording(void) {
    return recording;
}

void perf_sample(const registers_t *regs, const process_t *p) {
    if (!recording)
        return;
    if (sample_count >= PERF_MAX_SAMPLES) {
        samples_lost++;
        return;
    }
    perf_sample_t *s = &samples[sample_count++];
    s->eip  = regs->eip;
    s->pid  = p->pid;
    s->user = (regs->cs & 3) == 3;
    strncpy(s->name, p->name, sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
}

int perf_sample_count(void) {
    return sample_count;
}

const perf_sample_t *perf_sample_get(int index) {
    return index >= 0 && index < sample_count ? &samples[index] : 0;
}

uint32_t perf_samples_lost(void) {
    return samples_lost;
}
