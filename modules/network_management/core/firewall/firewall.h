#ifndef FIREWALL_H
#define FIREWALL_H

/*
 * firewall.h - Packet filter: rules that decide which packets may pass.
 *
 * Every IP packet passes a CHAIN of rules, checked from the top; the
 * first rule that matches decides. If none does, the chain's POLICY
 * decides:
 *
 *   INPUT    packets for this machine     (after fragments are put together)
 *   OUTPUT   packets this machine sends
 *
 *   a rule:  match  -p tcp  -s 10.0.2.0/24  --dport 22  --syn  -i eth0
 *            target ACCEPT | DROP (silently) | REJECT (tell the sender) | LOG
 *
 * `iptables -A INPUT -p tcp --dport 80 -j DROP` and the web server is
 * gone from the outside. Each rule counts the packets it matched.
 *
 * INPUT runs after reassembly on purpose: a rule about ports must see
 * the TCP/UDP header, which only the FIRST fragment carries
 * (security/firewall_bypass_fragments).
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

#define FW_MAX_RULES 16

typedef enum { FW_INPUT, FW_OUTPUT, FW_CHAINS } fw_chain_t;
typedef enum { FW_ACCEPT, FW_DROP, FW_REJECT, FW_LOG } fw_target_t;

typedef struct {
    int used;
    fw_chain_t chain;
    uint8_t protocol;               // 0 = any, 1 ICMP, 6 TCP, 17 UDP
    uint32_t src, src_mask;         // host order; mask 0 = any
    uint32_t dst, dst_mask;
    uint16_t sport, dport;          // 0 = any
    int syn_only;                   // --syn: only a TCP SYN (a new connection)
    char iface[NETDEV_NAME_LEN];    // "" = any
    fw_target_t target;
    uint32_t packets, bytes;        // what it matched
} fw_rule_t;

typedef struct {
    uint32_t dropped, rejected, logged;
} fw_stats_t;

// Should this IP packet pass? p->data points at the IP header.
// 'dev': the interface it came in on / goes out of. 1 = pass, 0 = it was
// dropped or rejected (the caller frees it).
int firewall_check(fw_chain_t chain, packet_t *p, netdev_t *dev);

// The table, in order, for iptables. Positions count from 1 per chain.
int firewall_append(const fw_rule_t *rule);                 // 0 or -NET_ENOBUFS
int firewall_insert(const fw_rule_t *rule);                 // at the top of its chain
int firewall_delete(fw_chain_t chain, int position);        // 0 or -NET_EINVAL
void firewall_flush(int chain);                             // -1 = every chain
void firewall_zero(void);                                   // counters to 0
void firewall_set_policy(fw_chain_t chain, fw_target_t policy);
fw_target_t firewall_policy(fw_chain_t chain);
uint32_t firewall_policy_packets(fw_chain_t chain);         // decided by the policy
const fw_rule_t *firewall_rule(int slot);                   // 0 .. FW_MAX_RULES-1, in order
const fw_stats_t *firewall_stats(void);

const char *firewall_chain_name(fw_chain_t chain);
const char *firewall_target_name(fw_target_t target);

#endif
