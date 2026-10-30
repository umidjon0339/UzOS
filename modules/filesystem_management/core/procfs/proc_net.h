#ifndef PROC_NET_H
#define PROC_NET_H

/*
 * proc_net.h - The text of the files in /proc/net.
 *
 *   /proc/net/dev     every interface's packet and byte counters
 *   /proc/net/arp     the ARP cache
 *   /proc/net/route   the routing table (addresses in hex, as Linux)
 *   /proc/net/tcp     every TCP connection and listener
 *   /proc/net/udp     every bound UDP port
 *   /proc/net/unix    every AF_UNIX socket
 *   /proc/net/snmp    the protocol counters (netstat -s reads these)
 */

#include "modules/filesystem_management/core/procfs/procfs.h"  // proc_buf_t

typedef enum {
    PROC_NET_DEV, PROC_NET_ARP, PROC_NET_ROUTE, PROC_NET_TCP,
    PROC_NET_UDP, PROC_NET_UNIX, PROC_NET_SNMP, PROC_NET_FILES,
} proc_net_file_t;

// "dev", "arp", ...
const char *proc_net_name(proc_net_file_t which);

// Write the text of /proc/net/<which>.
void proc_net(proc_buf_t *buf, proc_net_file_t which);

#endif
