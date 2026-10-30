/*
 * cmd_iptables.c - `iptables`: the packet filter's rules.
 *
 *   iptables -L [-v]                                   list the chains (-v: counters)
 *   iptables -A INPUT -p tcp --dport 80 -j DROP        append a rule
 *   iptables -I INPUT -s 10.0.2.2 -j ACCEPT            insert at the top
 *   iptables -A INPUT -p tcp --syn -i eth0 -j REJECT   refuse new connections from outside
 *   iptables -D INPUT 1                                delete rule 1 of INPUT
 *   iptables -P INPUT DROP                             the policy: what no rule decides
 *   iptables -F [CHAIN]                                delete every rule
 *   iptables -Z                                        counters back to 0
 *
 *   matches: -p tcp|udp|icmp  -s A.B.C.D[/N]  -d A.B.C.D[/N]  --sport N
 *            --dport N  --syn  -i dev (INPUT)  -o dev (OUTPUT)
 *   targets: -j ACCEPT | DROP | REJECT | LOG
 *
 * Changing the rules needs CAP_NET_ADMIN. Careful with `-P INPUT DROP`:
 * every answer from the outside is dropped too (UzOS keeps no
 * connection state to tell answers apart).
 *
 * Uses: firewall_* (core/firewall), inet_* (core/inet), caps_require
 *       (security_management), arg_next, atoi, strcmp, strcpy, ui_*.
 */

#include "modules/network_management/commands/iptables/cmd_iptables.h"  // this command
#include "modules/network_management/core/firewall/firewall.h"      // firewall_*
#include "modules/network_management/core/inet/inet.h"              // inet_parse, inet_format, inet_mask
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strcmp, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE "iptables -L [-v] | -A|-I CHAIN rule -j TARGET | -D CHAIN n | -P CHAIN TARGET | -F | -Z"

static int chain_from(const char *text, fw_chain_t *out) {
    if (strcmp(text, "INPUT") == 0)  { *out = FW_INPUT;  return 1; }
    if (strcmp(text, "OUTPUT") == 0) { *out = FW_OUTPUT; return 1; }
    return 0;
}

static int target_from(const char *text, fw_target_t *out) {
    for (int t = FW_ACCEPT; t <= FW_LOG; t++) {
        if (strcmp(text, firewall_target_name((fw_target_t)t)) == 0) {
            *out = (fw_target_t)t;
            return 1;
        }
    }
    return 0;
}

// "10.0.2.0/24" -> address and mask (no /N = one host).
static int cidr_from(char *text, uint32_t *ip, uint32_t *mask) {
    int prefix = 32;
    for (char *c = text; *c; c++) {
        if (*c == '/') {
            *c = '\0';
            prefix = atoi(c + 1);
        }
    }
    *mask = inet_mask(prefix);
    return prefix >= 0 && prefix <= 32 && inet_parse(text, ip);
}

static const char *proto_name(uint8_t p) {
    return p == 1 ? "icmp" : p == 6 ? "tcp" : p == 17 ? "udp" : "all";
}

/* ---------- -L ---------- */

static void addr_text(char *out, int size, uint32_t ip, uint32_t mask) {
    char a[INET_ADDR_TEXT];
    inet_format(ip, a);
    if (!mask)
        format(out, size, "anywhere");
    else if (mask == 0xFFFFFFFF)
        format(out, size, "%s", a);
    else
        format(out, size, "%s/%d", a, inet_prefix(mask));
}

static void list(int verbose) {
    for (int chain = 0; chain < FW_CHAINS; chain++) {
        fw_target_t policy = firewall_policy((fw_chain_t)chain);
        ui_color(UI_TITLE);
        ui_printf("Chain %s (policy %s", firewall_chain_name((fw_chain_t)chain), firewall_target_name(policy));
        if (verbose)
            ui_printf(" %u packets", firewall_policy_packets((fw_chain_t)chain));
        ui_printf(")\n");
        ui_reset();
        ui_printf("%s%-4s %-7s %-5s %-18s %-18s %s\n", verbose ? " pkts  bytes " : "", "num", "target",
                  "prot", "source", "destination", "");
        int n = 0;
        for (int i = 0; i < FW_MAX_RULES; i++) {
            const fw_rule_t *r = firewall_rule(i);
            if (!r->used || r->chain != (fw_chain_t)chain)
                continue;
            char src[24], dst[24];
            addr_text(src, sizeof(src), r->src, r->src_mask);
            addr_text(dst, sizeof(dst), r->dst, r->dst_mask);
            if (verbose)
                ui_printf("%5u %6u ", r->packets, r->bytes);
            ui_printf("%-4d ", ++n);
            ui_color(r->target == FW_ACCEPT ? UI_GOOD : r->target == FW_LOG ? UI_WARNING : UI_BAD);
            ui_printf("%-7s ", firewall_target_name(r->target));
            ui_reset();
            ui_printf("%-5s %-18s %-18s", proto_name(r->protocol), src, dst);
            if (r->sport)    ui_printf(" spt:%u", r->sport);
            if (r->dport)    ui_printf(" dpt:%u", r->dport);
            if (r->syn_only) ui_printf(" flags:SYN");
            if (r->iface[0]) ui_printf(" %s %s", chain == FW_INPUT ? "in" : "out", r->iface);
            ui_printf("\n");
        }
        ui_printf("\n");
    }
    const fw_stats_t *s = firewall_stats();
    ui_color(UI_DIM);
    ui_printf("dropped %u, rejected %u, logged %u (LOG lines go to the audit log)\n",
              s->dropped, s->rejected, s->logged);
    ui_reset();
}

/* ---------- -A / -I ---------- */

static void add(int insert, const char *rest) {
    char word[24];
    fw_rule_t r = { 0 };
    int have_target = 0, ok = 1;
    rest = arg_next(rest, word, sizeof(word));
    if (!chain_from(word, &r.chain)) {
        ui_usage(USAGE);
        return;
    }
    for (rest = arg_next(rest, word, sizeof(word)); word[0] && ok; rest = arg_next(rest, word, sizeof(word))) {
        char value[24];
        if (strcmp(word, "--syn") == 0) {
            r.syn_only = 1;
            r.protocol = 6;
            continue;
        }
        rest = arg_next(rest, value, sizeof(value));
        if (strcmp(word, "-p") == 0) {
            r.protocol = !strcmp(value, "tcp") ? 6 : !strcmp(value, "udp") ? 17 : !strcmp(value, "icmp") ? 1 : 0;
            ok = r.protocol || !strcmp(value, "all");
        } else if (strcmp(word, "-s") == 0)        ok = cidr_from(value, &r.src, &r.src_mask);
        else if (strcmp(word, "-d") == 0)        ok = cidr_from(value, &r.dst, &r.dst_mask);
        else if (strcmp(word, "--sport") == 0)   ok = (r.sport = (uint16_t)atoi(value)) != 0;
        else if (strcmp(word, "--dport") == 0)   ok = (r.dport = (uint16_t)atoi(value)) != 0;
        else if (strcmp(word, "-i") == 0 || strcmp(word, "-o") == 0) strcpy(r.iface, value);
        else if (strcmp(word, "-j") == 0)        ok = have_target = target_from(value, &r.target);
        else                                     ok = 0;
    }
    if (!ok || !have_target || ((r.sport || r.dport) && r.protocol != 6 && r.protocol != 17)) {
        ui_usage(USAGE);
        ui_hint("--sport / --dport need -p tcp or -p udp");
        return;
    }
    if ((insert ? firewall_insert(&r) : firewall_append(&r)) < 0)
        ui_error("iptables", "the rule table is full");
}

void cmd_iptables(const char *args) {
    char word[12], chain_text[12], value[12];
    const char *rest = arg_next(args, word, sizeof(word));
    fw_chain_t chain;
    fw_target_t target;

    if (!word[0] || strcmp(word, "-L") == 0 || strcmp(word, "-nL") == 0) {
        arg_next(rest, value, sizeof(value));
        list(strcmp(value, "-v") == 0);
        return;
    }
    if (strcmp(word, "-vL") == 0 || strcmp(word, "-Lv") == 0) {
        list(1);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "iptables"))
        return;
    if (strcmp(word, "-A") == 0 || strcmp(word, "-I") == 0) {
        add(word[1] == 'I', rest);
    } else if (strcmp(word, "-D") == 0) {
        rest = arg_next(rest, chain_text, sizeof(chain_text));
        arg_next(rest, value, sizeof(value));
        if (!chain_from(chain_text, &chain) || firewall_delete(chain, atoi(value)) < 0)
            ui_error("iptables", "no such rule (iptables -L shows the numbers)");
    } else if (strcmp(word, "-P") == 0) {
        rest = arg_next(rest, chain_text, sizeof(chain_text));
        arg_next(rest, value, sizeof(value));
        if (!chain_from(chain_text, &chain) || !target_from(value, &target) ||
            (target != FW_ACCEPT && target != FW_DROP))
            ui_usage("iptables -P INPUT|OUTPUT ACCEPT|DROP");
        else
            firewall_set_policy(chain, target);
    } else if (strcmp(word, "-F") == 0) {
        arg_next(rest, chain_text, sizeof(chain_text));
        firewall_flush(chain_from(chain_text, &chain) ? (int)chain : -1);
    } else if (strcmp(word, "-Z") == 0) {
        firewall_zero();
    } else {
        ui_usage(USAGE);
    }
}
