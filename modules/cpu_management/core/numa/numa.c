/*
 * numa.c - One node, memory policies, and the mbind() ranges.
 *
 * Uses: pmm_get_max_blocks / pmm_get_free_blocks (memory_management),
 *       vmm_walk (memory_management) to see whether a page is mapped,
 *       USER_SPACE_START / USER_SPACE_END (memory_layout.h),
 *       affinity_parse_list (core/affinity) for node lists.
 */

#include "modules/cpu_management/core/numa/numa.h"                  // this part
#include "modules/cpu_management/core/affinity/affinity.h"          // affinity_parse_list
#include "modules/memory_management/core/pmm/pmm.h"                 // pmm_get_*_blocks
#include "modules/memory_management/core/vmm/vmm.h"                 // vmm_walk, USER_SPACE_*
#include "lib/string/string.h"                                      // strcmp, memset

// mbind() ranges, one small table per process slot.
typedef struct {
    uint32_t start, length;     // length 0 = unused
    int      mode;
    uint32_t nodes;
} numa_range_t;

static numa_range_t ranges[MAX_PROCESSES][NUMA_RANGES];

static const char *mode_names[MPOL_MODE_COUNT] = {
    "default", "preferred", "bind", "interleave", "local"
};

uint32_t numa_online_nodes(void)        { return (1u << NUMA_NODES) - 1; }
int numa_node_of_cpu(int cpu)           { (void)cpu; return 0; }
uint32_t numa_cpus_of_node(int node)    { return node == 0 ? affinity_online_mask() : 0; }
uint32_t numa_node_pages(int node)      { return node == 0 ? pmm_get_max_blocks() : 0; }
uint32_t numa_node_free_pages(int node) { return node == 0 ? pmm_get_free_blocks() : 0; }

int numa_node_of_address(uint32_t addr) {
    vmm_walk_t walk;
    if (addr < USER_SPACE_START || addr >= USER_SPACE_END)
        return -1;
    if (!vmm_walk(addr, &walk))
        return -1;
    return 0;                   // every frame is on node 0
}

const char *numa_mode_name(int mode) {
    return mode >= 0 && mode < MPOL_MODE_COUNT ? mode_names[mode] : 0;
}

int numa_policy_ok(int mode, uint32_t nodes) {
    if (mode < 0 || mode >= MPOL_MODE_COUNT)
        return 0;
    if (nodes & ~numa_online_nodes())
        return 0;                           // a node that does not exist
    if (mode == MPOL_DEFAULT || mode == MPOL_LOCAL)
        return nodes == 0;                  // these choose the node themselves
    return nodes != 0;
}

static numa_range_t *ranges_of(const process_t *p) {
    return ranges[p - process_get_table()];
}

void numa_task_fork(process_t *child, const process_t *parent) {
    child->mempolicy       = parent ? parent->mempolicy : MPOL_DEFAULT;
    child->mempolicy_nodes = parent ? parent->mempolicy_nodes : 0;
    memset(ranges_of(child), 0, sizeof(ranges[0]));
}

int numa_bind_range(process_t *p, uint32_t start, uint32_t length, int mode,
                    uint32_t nodes) {
    numa_range_t *r = ranges_of(p);
    for (int i = 0; i < NUMA_RANGES; i++) {
        if (r[i].length && r[i].start != start)
            continue;                       // used by another range
        r[i] = (numa_range_t){ start, length, mode, nodes };
        return 1;
    }
    return 0;
}

void numa_policy_at(const process_t *p, uint32_t addr, int *mode, uint32_t *nodes) {
    const numa_range_t *r = ranges_of(p);
    for (int i = 0; i < NUMA_RANGES; i++) {
        if (r[i].length && addr >= r[i].start && addr - r[i].start < r[i].length) {
            *mode = r[i].mode;
            *nodes = r[i].nodes;
            return;
        }
    }
    *mode = p->mempolicy;
    *nodes = p->mempolicy_nodes;
}

uint32_t numa_parse_nodes(const char *text) {
    if (strcmp(text, "all") == 0)
        return numa_online_nodes();
    return affinity_parse_list(text);       // same syntax: "0", "0-1", "0,2"
}
