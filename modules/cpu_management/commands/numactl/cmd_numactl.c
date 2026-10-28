/*
 * cmd_numactl.c - `numactl`: NUMA nodes, and run a command placed on them.
 *
 *   numactl --hardware   (-H)    the nodes: CPUs, memory, distances
 *   numactl --show       (-s)    the shell's memory policy and CPUs
 *
 *   numactl [options] command    run it with a placement:
 *     --membind=NODES     -m     memory only from these nodes   (MPOL_BIND)
 *     --preferred=NODE    -p     memory from here first         (MPOL_PREFERRED)
 *     --interleave=NODES  -i     memory spread over the nodes   (MPOL_INTERLEAVE)
 *     --localalloc        -l     memory of the CPU's own node   (MPOL_LOCAL)
 *     --cpunodebind=NODES -N     only the CPUs of these nodes   (affinity)
 *     --physcpubind=CPUS  -C     only these CPUs                (affinity)
 *   NODES / CPUS: "0", "0-1", "0,1" or "all".
 *
 * UzOS has one node (core/numa), so node 0 is the only one that works;
 * asking for node 1 is refused, as on a machine without a node 1.
 *
 * Uses: numa_* (core/numa), affinity_* (core/affinity), numactl_run_child
 *       (run_child.c, this folder), the current process (process_management), ui_*
 *       (ui), arg_next / strncmp / strcmp (lib).
 */

#include "modules/cpu_management/commands/numactl/cmd_numactl.h"    // this command
#include "modules/cpu_management/core/numa/numa.h"                  // numa_*
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_*
#include "modules/cpu_management/commands/numactl/run_child.h"          // numactl_run_child (this folder)
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcmp, strncmp, strlen

#define KB_PER_PAGE 4

// The placement the options asked for.
typedef struct {
    int      mode;              // MPOL_*, or -1: keep the inherited one
    uint32_t nodes;
    uint32_t cpus;              // 0 = keep the inherited mask
} placement_t;

static void hardware(void) {
    char cpus[32];
    ui_printf("available: %d nodes (0)\n", NUMA_NODES);
    for (int node = 0; node < NUMA_NODES; node++) {
        affinity_list(numa_cpus_of_node(node), cpus, sizeof(cpus));
        ui_printf("node %d cpus: %s\n", node, cpus);
        ui_printf("node %d size: %u MB\n", node, numa_node_pages(node) * KB_PER_PAGE / 1024);
        ui_printf("node %d free: %u MB\n", node, numa_node_free_pages(node) * KB_PER_PAGE / 1024);
    }
    vga_print_str("node distances:\nnode   0\n");
    ui_printf("  0:  %d\n", NUMA_LOCAL_DISTANCE);
}

static void show(void) {
    process_t *self = scheduler_get_current_process();
    char cpus[32], nodes[32];
    affinity_list(affinity_get(self), cpus, sizeof(cpus));
    affinity_list(self->mempolicy_nodes ? self->mempolicy_nodes : numa_online_nodes(),
                  nodes, sizeof(nodes));

    ui_printf("policy: %s\n", numa_mode_name(self->mempolicy));
    ui_printf("preferred node: %s\n", self->mempolicy == MPOL_PREFERRED ? nodes : "current");
    ui_printf("physcpubind: %s\n", cpus);
    vga_print_str("cpubind: 0\nnodebind: 0\n");
    ui_printf("membind: %s\n", nodes);
}

// Runs before the child may run (run_child.c, this folder).
static int place_child(process_t *child, void *arg) {
    placement_t *want = arg;
    if (want->mode >= 0) {
        child->mempolicy = want->mode;
        child->mempolicy_nodes = want->nodes;
    }
    if (want->cpus && !affinity_set(scheduler_get_current_process(), child->pid, want->cpus)) {
        ui_error("numactl", "cannot bind to those CPUs");
        return 0;
    }
    return 1;
}

// "--membind=0" or "-m 0": the value of an option. 'rest' moves past
// the separate value word if there was one.
static const char *value_of(const char *word, const char *long_name, const char **rest,
                            char *buf, int size) {
    int len = strlen(long_name);
    if (strncmp(word, long_name, len) == 0 && word[len] == '=')
        return word + len + 1;
    *rest = arg_next(*rest, buf, size);
    return buf;
}

void cmd_numactl(const char *args) {
    placement_t want = { -1, 0, 0 };
    char word[32], value_buf[32];
    const char *rest = args;

    while (1) {
        const char *next = arg_next(rest, word, sizeof(word));
        if (!word[0])
            break;
        if (strcmp(word, "--hardware") == 0 || strcmp(word, "-H") == 0) {
            hardware();
            return;
        }
        if (strcmp(word, "--show") == 0 || strcmp(word, "-s") == 0) {
            show();
            return;
        }
        if (word[0] != '-')
            break;                          // the command starts here

        const char *value = 0;
        rest = next;
        if (strncmp(word, "--membind", 9) == 0 || strcmp(word, "-m") == 0) {
            value = value_of(word, "--membind", &rest, value_buf, sizeof(value_buf));
            want.mode = MPOL_BIND;
        } else if (strncmp(word, "--preferred", 11) == 0 || strcmp(word, "-p") == 0) {
            value = value_of(word, "--preferred", &rest, value_buf, sizeof(value_buf));
            want.mode = MPOL_PREFERRED;
        } else if (strncmp(word, "--interleave", 12) == 0 || strcmp(word, "-i") == 0) {
            value = value_of(word, "--interleave", &rest, value_buf, sizeof(value_buf));
            want.mode = MPOL_INTERLEAVE;
        } else if (strcmp(word, "--localalloc") == 0 || strcmp(word, "-l") == 0) {
            want.mode = MPOL_LOCAL;
            continue;
        } else if (strncmp(word, "--cpunodebind", 13) == 0 || strcmp(word, "-N") == 0) {
            uint32_t nodes = numa_parse_nodes(value_of(word, "--cpunodebind", &rest,
                                                       value_buf, sizeof(value_buf)));
            if (!nodes || (nodes & ~numa_online_nodes())) {
                ui_error("numactl", "no such node (numactl --hardware lists them)");
                return;
            }
            want.cpus = numa_cpus_of_node(0);
            continue;
        } else if (strncmp(word, "--physcpubind", 13) == 0 || strcmp(word, "-C") == 0) {
            want.cpus = affinity_parse_list(value_of(word, "--physcpubind", &rest,
                                                     value_buf, sizeof(value_buf)));
            if (!want.cpus) {
                ui_error("numactl", "not a CPU list");
                return;
            }
            continue;
        } else {
            ui_error("numactl", "unknown option");
            return;
        }

        want.nodes = numa_parse_nodes(value);
        if (!numa_policy_ok(want.mode, want.nodes)) {
            ui_error("numactl", "no such node (numactl --hardware lists them)");
            return;
        }
    }

    while (*rest == ' ')
        rest++;
    if (!*rest) {
        ui_usage("numactl --hardware | --show | [--membind=N|--preferred=N|--interleave=N|"
                 "--localalloc] [--cpunodebind=N|--physcpubind=C] command");
        return;
    }
    if (numactl_run_child(rest, place_child, &want) < 0)
        ui_error("numactl", "the command did not run");
}
