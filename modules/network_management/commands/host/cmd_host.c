/*
 * cmd_host.c - `host`: a short DNS answer.
 *
 *   host example.com           example.com has address 93.184.215.14
 *   host -t mx gmail.com       gmail.com mail is handled by 5 gmail-smtp-in.l.google.com
 *   host 8.8.8.8               8.8.8.8.in-addr.arpa domain name pointer dns.google
 *
 * A reverse lookup (address -> name) asks for the PTR record of the
 * address written backwards under in-addr.arpa: 8.8.4.4 -> 4.4.8.8.in-addr.arpa.
 *
 * Uses: dns_query / dns_* (core/dns), inet_* (core/inet), arg_next,
 *       format, strcmp, ui_*.
 */

#include "modules/network_management/commands/host/cmd_host.h"      // this command
#include "modules/network_management/core/dns/dns.h"                // dns_query, DNS_TYPE_*
#include "modules/network_management/core/inet/inet.h"              // inet_parse, inet_format
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strcmp
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE "host [-t a|mx|ns|txt|cname|aaaa|ptr] name | address"

static uint16_t type_from(const char *text) {
    static const struct { const char *name; uint16_t type; } types[] = {
        { "a", DNS_TYPE_A }, { "mx", DNS_TYPE_MX }, { "ns", DNS_TYPE_NS },
        { "txt", DNS_TYPE_TXT }, { "cname", DNS_TYPE_CNAME },
        { "aaaa", DNS_TYPE_AAAA }, { "ptr", DNS_TYPE_PTR },
    };
    for (unsigned i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        const char *a = text, *b = types[i].name;
        while (*a && (*a | 32) == *b)       // case-insensitive
            a++, b++;
        if (!*a && !*b)
            return types[i].type;
    }
    return 0;
}

static void print_record(const dns_record_t *r) {
    char ip[INET_ADDR_TEXT];
    switch (r->type) {
    case DNS_TYPE_A:
        inet_format(r->ip, ip);
        ui_printf("%s has address ", r->name);
        ui_color(UI_ACCENT);
        ui_printf("%s\n", ip);
        ui_reset();
        break;
    case DNS_TYPE_AAAA:  ui_printf("%s has IPv6 address %s\n", r->name, r->text); break;
    case DNS_TYPE_CNAME: ui_printf("%s is an alias for %s.\n", r->name, r->text); break;
    case DNS_TYPE_MX:    ui_printf("%s mail is handled by %u %s.\n", r->name, r->preference, r->text); break;
    case DNS_TYPE_NS:    ui_printf("%s name server %s.\n", r->name, r->text); break;
    case DNS_TYPE_PTR:   ui_printf("%s domain name pointer %s.\n", r->name, r->text); break;
    case DNS_TYPE_TXT:   ui_printf("%s descriptive text \"%s\"\n", r->name, r->text); break;
    }
}

void cmd_host(const char *args) {
    char word[DNS_NAME_MAX], name[DNS_NAME_MAX] = "";
    uint16_t type = DNS_TYPE_A;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-t") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            type = type_from(word);
        } else {
            strcpy(name, word);
        }
    }
    if (!name[0] || !type) {
        ui_usage(USAGE);
        return;
    }

    uint32_t ip;
    if (inet_parse(name, &ip)) {            // an address: ask backwards
        format(name, sizeof(name), "%u.%u.%u.%u.in-addr.arpa",
               ip & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, ip >> 24);
        type = DNS_TYPE_PTR;
    }

    static dns_result_t result;
    int rcode = dns_query(name, type, 0, &result);
    if (rcode != DNS_OK) {
        ui_color(UI_WARNING);
        ui_printf("Host %s %s\n", name, dns_error_text(rcode));
        ui_reset();
        return;
    }
    for (int i = 0; i < result.count; i++)
        print_record(&result.answers[i]);
}
