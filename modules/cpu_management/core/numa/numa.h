#ifndef NUMA_H
#define NUMA_H

/*
 * numa.h - NUMA: which memory is close to which CPU.
 *
 * Big machines have several "nodes". Each node is some CPUs plus the
 * RAM wired straight to them:
 *
 *     node 0: CPU 0-7  + 64 GB        node 1: CPU 8-15 + 64 GB
 *          \___________ slower link between nodes ________/
 *
 * Memory on your own node is fast; memory on the other node is slower
 * (the "distance": 10 = local, 20 = twice as far). So Linux lets a
 * program choose where its memory comes from: a MEMORY POLICY.
 *
 *   MPOL_DEFAULT      the policy of the process, or "local"
 *   MPOL_PREFERRED    try this node first, else any
 *   MPOL_BIND         only these nodes; fail if they are full
 *   MPOL_INTERLEAVE   page 1 on node A, page 2 on node B, ...
 *   MPOL_LOCAL        the node of the CPU that touches the page
 *
 * A PC like the one QEMU emulates has ONE node: node 0 = CPU 0 + all
 * RAM. UzOS keeps the policies, checks them, and answers every question
 * about nodes truthfully; there is just never a second node to move a
 * page to.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define NUMA_NODES        1     // node 0 only
#define NUMA_LOCAL_DISTANCE 10  // Linux's "same node" distance

#define MPOL_DEFAULT      0
#define MPOL_PREFERRED    1
#define MPOL_BIND         2
#define MPOL_INTERLEAVE   3
#define MPOL_LOCAL        4
#define MPOL_MODE_COUNT   5

// get_mempolicy() flags
#define MPOL_F_NODE         1   // return a node number instead of a mode
#define MPOL_F_ADDR         2   // the policy (or node) of one address
#define MPOL_F_MEMS_ALLOWED 4   // the nodes this process may use

// mbind() flags
#define MPOL_MF_STRICT    1     // fail if a page is on a wrong node
#define MPOL_MF_MOVE      2     // move the pages that are on a wrong node
#define MPOL_MF_MOVE_ALL  4

#define NUMA_RANGES 4           // mbind() ranges remembered per process

// Bit n set = node n exists: 0b1.
uint32_t numa_online_nodes(void);

// The node of CPU 'cpu' (0), and the CPUs of node 'node' (mask 0b1).
int numa_node_of_cpu(int cpu);
uint32_t numa_cpus_of_node(int node);

// RAM of a node, in 4 KiB pages: in total and free right now.
uint32_t numa_node_pages(int node);
uint32_t numa_node_free_pages(int node);

// The node holding the page at user address 'addr': 0, or -1 if the
// address is outside user space or not mapped.
int numa_node_of_address(uint32_t addr);

// "default", "bind", ...; NULL if not a mode.
const char *numa_mode_name(int mode);

// Is (mode, nodes) a valid policy? DEFAULT and LOCAL take no nodes;
// PREFERRED / BIND / INTERLEAVE need at least one; every node must exist.
int numa_policy_ok(int mode, uint32_t nodes);

// Give 'child' its parent's policy (or the default) and no ranges.
void numa_task_fork(process_t *child, const process_t *parent);

// mbind(): remember a policy for [start, start + length) of 'p'.
// 0 if out of room (NUMA_RANGES per process).
int numa_bind_range(process_t *p, uint32_t start, uint32_t length, int mode,
                    uint32_t nodes);

// The policy in force at 'addr' for 'p': an mbind() range, else the
// process's own policy.
void numa_policy_at(const process_t *p, uint32_t addr, int *mode, uint32_t *nodes);

// Read a node list ("0", "all") into a mask; 0 if it is not one.
uint32_t numa_parse_nodes(const char *text);

#endif
