/*
 * firewall.c - Match each packet against the rules of its chain.
 *
 * The rules live in one array, kept in order: a chain's rules are the
 * used entries with that chain, top to bottom. REJECT answers like a
 * closed port would (TCP: a RST; anything else: ICMP port unreachable),
 * so the sender stops at once instead of waiting.
 *
 * Uses: ipv4_header_t (core/ipv4), icmp_send_error (core/icmp),
 *       audit_log (security_management), cpu_irq_save, ntohl / ntohs
 *       (core/inet), memcpy / strcmp (lib/string).
 */

#include "modules/network_management/core/firewall/firewall.h"   // this file
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_header_t, IPPROTO_*
#include "modules/network_management/core/icmp/icmp.h"           // icmp_send_error
#include "modules/network_management/core/tcp/tcp.h"             // tcp_header_t, TCP_SYN, TCP_ACK
#include "modules/network_management/core/inet/inet.h"           // ntohl, ntohs, NET_E*
#include "modules/security_management/core/audit/audit.h"        // audit_log
#include "arch/x86/cpu/cpu.h"                                     // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                    // memcpy, strcmp

static fw_rule_t rules[FW_MAX_RULES];
static fw_target_t policy[FW_CHAINS] = { FW_ACCEPT, FW_ACCEPT };
static uint32_t policy_packets[FW_CHAINS];
static fw_stats_t stats;

const char *firewall_chain_name(fw_chain_t chain) {
    return chain == FW_INPUT ? "INPUT" : "OUTPUT";
}

const char *firewall_target_name(fw_target_t target) {
    static const char *names[] = { "ACCEPT", "DROP", "REJECT", "LOG" };
    return names[target];
}

/* ---------- Matching ---------- */

// The ports and TCP flags, if the packet has them (the first 4 bytes
// after the IP header are the ports in TCP and UDP alike).
static void transport(const packet_t *p, uint16_t *sport, uint16_t *dport, uint8_t *tcp_flags) {
    const ipv4_header_t *ip = (const ipv4_header_t *)p->data;
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    *sport = *dport = 0;
    *tcp_flags = 0;
    if (ip->protocol != IPPROTO_TCP && ip->protocol != IPPROTO_UDP)
        return;
    if (p->len < hlen + 4 || (ntohs(ip->frag) & IPV4_OFFSET_MASK))
        return;                             // a later fragment: no header here
    const uint8_t *t = p->data + hlen;
    *sport = (uint16_t)((t[0] << 8) | t[1]);
    *dport = (uint16_t)((t[2] << 8) | t[3]);
    if (ip->protocol == IPPROTO_TCP && p->len >= hlen + TCP_HLEN)
        *tcp_flags = ((const tcp_header_t *)t)->flags;
}

static int matches(const fw_rule_t *r, const packet_t *p, netdev_t *dev) {
    const ipv4_header_t *ip = (const ipv4_header_t *)p->data;
    uint16_t sport, dport;
    uint8_t flags;
    transport(p, &sport, &dport, &flags);
    if (r->protocol && r->protocol != ip->protocol)
        return 0;
    if ((ntohl(ip->src) & r->src_mask) != (r->src & r->src_mask))
        return 0;
    if ((ntohl(ip->dst) & r->dst_mask) != (r->dst & r->dst_mask))
        return 0;
    if ((r->sport && r->sport != sport) || (r->dport && r->dport != dport))
        return 0;
    if (r->syn_only && (!(flags & TCP_SYN) || (flags & TCP_ACK)))
        return 0;
    if (r->iface[0] && (!dev || strcmp(r->iface, dev->name) != 0))
        return 0;
    return 1;
}

// REJECT: answer as a closed port would.
static void reject(packet_t *p) {
    p->net_header = p->data;                // icmp_send_error quotes the IP header from here
    icmp_send_error(p, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH);
}

int firewall_check(fw_chain_t chain, packet_t *p, netdev_t *dev) {
    fw_target_t verdict = policy[chain];
    int decided = 0;
    for (int i = 0; i < FW_MAX_RULES && !decided; i++) {
        fw_rule_t *r = &rules[i];
        if (!r->used || r->chain != chain || !matches(r, p, dev))
            continue;
        r->packets++;
        r->bytes += p->len;
        if (r->target == FW_LOG) {          // LOG writes a line and goes on to the next rule
            stats.logged++;
            audit_log("firewall", chain == FW_INPUT ? "LOG: a packet came in" : "LOG: a packet went out");
            continue;
        }
        verdict = r->target;
        decided = 1;
    }
    if (!decided)
        policy_packets[chain]++;
    if (verdict == FW_DROP) {
        stats.dropped++;
        return 0;
    }
    if (verdict == FW_REJECT) {
        stats.rejected++;
        if (chain == FW_INPUT)
            reject(p);
        return 0;
    }
    return 1;
}

/* ---------- The table ---------- */

int firewall_append(const fw_rule_t *rule) {
    uint32_t flags = cpu_irq_save();
    int result = -NET_ENOBUFS;
    for (int i = 0; i < FW_MAX_RULES; i++) {
        if (!rules[i].used) {
            rules[i] = *rule;
            rules[i].used = 1;
            rules[i].packets = rules[i].bytes = 0;
            result = 0;
            break;
        }
    }
    cpu_irq_restore(flags);
    return result;
}

int firewall_insert(const fw_rule_t *rule) {
    uint32_t flags = cpu_irq_save();
    int result = -NET_ENOBUFS;
    // The used rules always sit together at the front, in order. Shift
    // them one down to make room at the top.
    int count = 0;
    while (count < FW_MAX_RULES && rules[count].used)
        count++;
    if (count < FW_MAX_RULES) {
        for (int i = count; i > 0; i--)
            rules[i] = rules[i - 1];
        rules[0] = *rule;
        rules[0].used = 1;
        rules[0].packets = rules[0].bytes = 0;
        result = 0;
    }
    cpu_irq_restore(flags);
    return result;
}

int firewall_delete(fw_chain_t chain, int position) {
    uint32_t flags = cpu_irq_save();
    int n = 0, result = -NET_EINVAL;
    for (int i = 0; i < FW_MAX_RULES; i++) {
        if (rules[i].used && rules[i].chain == chain && ++n == position) {
            for (int j = i; j < FW_MAX_RULES - 1; j++)    // close the gap: keep the order
                rules[j] = rules[j + 1];
            rules[FW_MAX_RULES - 1].used = 0;
            result = 0;
            break;
        }
    }
    cpu_irq_restore(flags);
    return result;
}

void firewall_flush(int chain) {
    uint32_t flags = cpu_irq_save();
    int out = 0;
    for (int i = 0; i < FW_MAX_RULES; i++)
        if (rules[i].used && chain >= 0 && (int)rules[i].chain != chain)
            rules[out++] = rules[i];        // keep the other chains' rules, in order
    for (int i = out; i < FW_MAX_RULES; i++)
        rules[i].used = 0;
    cpu_irq_restore(flags);
}

void firewall_zero(void) {
    for (int i = 0; i < FW_MAX_RULES; i++)
        rules[i].packets = rules[i].bytes = 0;
    policy_packets[FW_INPUT] = policy_packets[FW_OUTPUT] = 0;
}

void firewall_set_policy(fw_chain_t chain, fw_target_t target) { policy[chain] = target; }
fw_target_t firewall_policy(fw_chain_t chain)                   { return policy[chain]; }
uint32_t firewall_policy_packets(fw_chain_t chain)              { return policy_packets[chain]; }
const fw_stats_t *firewall_stats(void)                          { return &stats; }

const fw_rule_t *firewall_rule(int slot) {
    return slot >= 0 && slot < FW_MAX_RULES ? &rules[slot] : 0;
}
