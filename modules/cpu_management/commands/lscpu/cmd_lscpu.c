/*
 * cmd_lscpu.c - `lscpu [-e]`: the CPU, in Linux's lscpu layout.
 *
 *   lscpu       architecture, how many CPUs / cores / sockets, the
 *               model, the clock, the feature flags, the hypervisor,
 *               the NUMA nodes
 *   lscpu -e    one row per CPU: CPU NODE SOCKET CORE ONLINE MHZ
 *
 * Everything comes from cpuid (via core/info) and the measured clock
 * (core/tsc); the topology from core/affinity and core/numa.
 *
 * Uses: cpu_info / cpu_has / cpu_feature_name (core/info), tsc_mhz
 *       (core/tsc), affinity_* (core/affinity), numa_* (core/numa),
 *       cpuid_leaf (arch/x86/cpuid), ui_* (ui), strcmp (lib/string).
 */

#include "modules/cpu_management/commands/lscpu/cmd_lscpu.h"    // this command
#include "modules/cpu_management/core/info/cpu_info.h"          // cpu_info, cpu_has
#include "modules/cpu_management/core/tsc/tsc.h"                // tsc_mhz
#include "modules/cpu_management/core/affinity/affinity.h"      // affinity_*
#include "modules/cpu_management/core/numa/numa.h"              // numa_*
#include "arch/x86/cpuid/cpuid.h"                               // cpuid_leaf
#include "modules/ui/core/style/ui.h"                           // ui_*
#include "lib/string/string.h"                                  // strcmp, memcpy
#include "lib/format/format.h"                                  // format

#define VALUE_COLUMN 26     // where the values start
#define WRAP_AT      78     // flags wrap before the screen edge

// "  Model name:" padded so the value starts at VALUE_COLUMN.
static void row(int indent, const char *label, const char *value) {
    int used = indent + strlen(label) + 1;
    ui_color(UI_LABEL);
    for (int i = 0; i < indent; i++)
        vga_print_str(" ");
    ui_printf("%s:", label);
    ui_reset();
    for (int i = used; i < VALUE_COLUMN; i++)
        vga_print_str(" ");
    ui_printf("%s\n", value);
}

static void row_number(int indent, const char *label, uint32_t value) {
    char text[16];
    format(text, sizeof(text), "%u", value);
    row(indent, label, text);
}

// cpuid leaf 0x40000000: a hypervisor's name ("TCGTCGTCGTCG", "KVMKVMKVM").
static void hypervisor_vendor(char out[13]) {
    uint32_t regs[4];
    cpuid_leaf(0x40000000, regs);
    memcpy(out, &regs[1], 4);
    memcpy(out + 4, &regs[2], 4);
    memcpy(out + 8, &regs[3], 4);
    out[12] = '\0';
}

static void print_flags(void) {
    int column = VALUE_COLUMN;
    ui_color(UI_LABEL);
    vga_print_str("    Flags:");
    ui_reset();
    for (int i = 10; i < VALUE_COLUMN; i++)
        vga_print_str(" ");
    for (int f = 0; f < CPU_FEAT_COUNT; f++) {
        if (!cpu_has(f))
            continue;
        const char *name = cpu_feature_name(f);
        if (column + (int)strlen(name) + 1 > WRAP_AT) {     // next line, same column
            vga_print_str("\n");
            for (int i = 0; i < VALUE_COLUMN; i++)
                vga_print_str(" ");
            column = VALUE_COLUMN;
        }
        ui_printf("%s ", name);
        column += strlen(name) + 1;
    }
    vga_print_str("\n");
}

static void summary(void) {
    const cpu_info_t *cpu = cpu_info();
    char text[64];

    row(0, "Architecture", "i686");
    row(2, "CPU op-mode(s)", "32-bit");
    row(2, "Byte Order", "Little Endian");
    row_number(0, "CPU(s)", affinity_cpu_count());
    affinity_list(affinity_online_mask(), text, sizeof(text));
    row(2, "On-line CPU(s) list", text);
    row(0, "Vendor ID", cpu->vendor);
    row(2, "Model name", cpu->brand[0] ? cpu->brand : "-");
    row_number(4, "CPU family", cpu->family);
    row_number(4, "Model", cpu->model);
    row_number(4, "Thread(s) per core", 1);
    row_number(4, "Core(s) per socket", 1);
    row_number(4, "Socket(s)", 1);
    row_number(4, "Stepping", cpu->stepping);
    format(text, sizeof(text), "%u.0000  (measured with the TSC)", tsc_mhz());
    row(4, "CPU MHz", text);
    format(text, sizeof(text), "%u.00", 2 * tsc_mhz());
    row(4, "BogoMIPS", text);
    print_flags();

    if (cpu_has(CPU_FEAT_HYPERVISOR)) {
        hypervisor_vendor(text);
        row(0, "Virtualization features", "");
        row(2, "Hypervisor vendor", text[0] ? text : "unknown");
        row(2, "Virtualization type", "full");
    }

    row(0, "NUMA", "");
    row_number(2, "NUMA node(s)", NUMA_NODES);
    affinity_list(numa_cpus_of_node(0), text, sizeof(text));
    row(2, "NUMA node0 CPU(s)", text);
}

static void extended(void) {
    ui_color(UI_TITLE);
    vga_print_str("CPU NODE SOCKET CORE ONLINE    MAXMHZ\n");
    ui_reset();
    for (int n = 0; n < affinity_cpu_count(); n++)
        ui_printf("%3d %4d %6d %4d %6s %9u.0000\n", n, numa_node_of_cpu(n), 0, n,
                  (affinity_online_mask() >> n) & 1 ? "yes" : "no", tsc_mhz());
}

void cmd_lscpu(const char *args) {
    if (strcmp(args, "-e") == 0)
        extended();
    else if (args[0])
        ui_usage("lscpu [-e]");
    else
        summary();
}
