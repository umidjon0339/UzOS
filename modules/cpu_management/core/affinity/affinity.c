/*
 * affinity.c - CPU masks: check, store, print.
 *
 * Uses: process_get_by_pid (process_management), caps_has
 *       (security_management) for CAP_PRIORITY, format (lib/format).
 */

#include "modules/cpu_management/core/affinity/affinity.h"         // this part
#include "modules/security_management/core/caps/caps.h"            // caps_has, CAP_PRIORITY
#include "lib/format/format.h"                                     // format

uint32_t affinity_online_mask(void) {
    return (1u << CPU_COUNT) - 1;
}

int affinity_cpu_count(void) {
    return CPU_COUNT;
}

int affinity_current_cpu(void) {
    return 0;       // with more CPUs: read the local APIC id
}

uint32_t affinity_get(const process_t *p) {
    return p ? p->cpu_mask & affinity_online_mask() : 0;
}

int affinity_set(const process_t *caller, uint32_t pid, uint32_t mask) {
    process_t *target = process_get_by_pid(pid);
    uint32_t uid = caller ? caller->uid : 0;

    if (!target || target->is_system)
        return 0;
    if (target->uid != uid && !caps_has(uid, CAP_PRIORITY))
        return 0;
    if ((mask & affinity_online_mask()) == 0)
        return 0;                           // it could never run anywhere

    target->cpu_mask = mask;
    return 1;
}

void affinity_list(uint32_t mask, char *out, int size) {
    int len = 0;
    out[0] = '\0';
    for (int cpu = 0; cpu < 32; cpu++) {
        if (!(mask & (1u << cpu)))
            continue;
        int last = cpu;                     // find the end of this run: "0-3"
        while (last + 1 < 32 && (mask & (1u << (last + 1))))
            last++;
        if (last == cpu)
            len += format(out + len, size - len, "%s%d", len ? "," : "", cpu);
        else
            len += format(out + len, size - len, "%s%d-%d", len ? "," : "", cpu, last);
        cpu = last;
    }
}

// One number at *s, moving *s past it. -1 if there is none.
static int read_number(const char **s) {
    int n = -1;
    while (**s >= '0' && **s <= '9') {
        n = (n < 0 ? 0 : n * 10) + (**s - '0');
        (*s)++;
    }
    return n;
}

uint32_t affinity_parse_list(const char *text) {
    uint32_t mask = 0;
    const char *s = text;

    while (*s) {
        int first = read_number(&s), last = first;
        if (*s == '-') {
            s++;
            last = read_number(&s);
        }
        if (first < 0 || last < first || last > 31)
            return 0;
        for (int cpu = first; cpu <= last; cpu++)
            mask |= 1u << cpu;
        if (*s == ',')
            s++;
        else if (*s)
            return 0;
    }
    return mask;
}
