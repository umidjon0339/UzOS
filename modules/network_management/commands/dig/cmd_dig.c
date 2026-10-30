/*
 * cmd_dig.c - `dig`: show a DNS answer the way the protocol has it.
 *
 *   dig example.com
 *   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 4711
 *   ;; flags: qr rd ra; QUERY: 1, ANSWER: 1
 *
 *   ;; QUESTION SECTION:
 *   ;example.com.                 IN  A
 *
 *   ;; ANSWER SECTION:
 *   example.com.          300     IN  A   93.184.215.14
 *
 *   ;; Query time: 12 msec
 *   ;; SERVER: 10.0.2.3#53(10.0.2.3)
 *
 *   dig @8.8.8.8 example.com     ask another server
 *   dig gmail.com mx             another record type
 *   dig example.com +short       only the data
 *
 * Every line starting with ';' is a comment: dig's output is itself a
 * valid DNS "zone file" listing of the answer.
 *
 * Uses: dns_query / dns_* (core/dns), inet_* (core/inet), arg_next,
 *       strcmp, strcpy, ui_*.
 */

#include "modules/network_management/commands/dig/cmd_dig.h"        // this command
#include "modules/network_management/core/dns/dns.h"                // dns_query, DNS_TYPE_*
#include "modules/network_management/core/inet/inet.h"              // inet_parse, inet_format
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcmp, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE "dig [@server] name [a|mx|ns|txt|cname|aaaa|ptr] [+short]"

// Header flag bits and their names
static const struct { uint16_t bit; const char *name; } flag_names[] = {
    { 0x8000, "qr" }, { 0x0400, "aa" }, { 0x0200, "tc" }, { 0x0100, "rd" }, { 0x0080, "ra" },
};

static uint16_t type_from(const char *text) {
    static const uint16_t types[] = { DNS_TYPE_A, DNS_TYPE_MX, DNS_TYPE_NS, DNS_TYPE_TXT,
                                      DNS_TYPE_CNAME, DNS_TYPE_AAAA, DNS_TYPE_PTR };
    for (unsigned i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        const char *a = text, *b = dns_type_name(types[i]);
        while (*a && (*a & ~32) == *b)      // case-insensitive ("mx" = "MX")
            a++, b++;
        if (!*a && !*b)
            return types[i];
    }
    return 0;
}

static const char *status_name(int rcode) {
    switch (rcode) {
    case DNS_OK:       return "NOERROR";
    case DNS_FORMERR:  return "FORMERR";
    case DNS_SERVFAIL: return "SERVFAIL";
    case DNS_NXDOMAIN: return "NXDOMAIN";
    case DNS_REFUSED:  return "REFUSED";
    default:           return "NOERROR";
    }
}

// The record's data: an address, a name, or text.
static void print_data(const dns_record_t *r) {
    char ip[INET_ADDR_TEXT];
    if (r->type == DNS_TYPE_A) {
        inet_format(r->ip, ip);
        ui_printf("%s", ip);
    } else if (r->type == DNS_TYPE_MX) {
        ui_printf("%u %s.", r->preference, r->text);
    } else if (r->type == DNS_TYPE_TXT) {
        ui_printf("\"%s\"", r->text);
    } else if (r->type == DNS_TYPE_AAAA) {
        ui_printf("%s", r->text);
    } else {
        ui_printf("%s.", r->text);
    }
}

void cmd_dig(const char *args) {
    char word[DNS_NAME_MAX], name[DNS_NAME_MAX] = "";
    uint16_t type = DNS_TYPE_A;
    uint32_t server = 0;
    int short_form = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] == '@') {
            if (!inet_parse(word + 1, &server)) {
                ui_usage(USAGE);
                return;
            }
        } else if (strcmp(word, "+short") == 0) {
            short_form = 1;
        } else if (name[0] && type_from(word)) {
            type = type_from(word);
        } else {
            strcpy(name, word);
        }
    }
    if (!name[0]) {
        ui_usage(USAGE);
        return;
    }

    static dns_result_t r;
    int rcode = dns_query(name, type, server, &r);
    char ip[INET_ADDR_TEXT];
    inet_format(r.server, ip);

    if (short_form) {
        for (int i = 0; i < r.count; i++) {
            print_data(&r.answers[i]);
            ui_printf("\n");
        }
        return;
    }

    ui_color(UI_DIM);
    ui_printf("; <<>> DiG (UzOS) <<>> %s%s %s\n", server ? "@" : "", server ? ip : "", name);
    ui_reset();
    if (rcode < 0 && rcode != DNS_ERR_NODATA) {
        ui_color(UI_WARNING);
        ui_printf(";; %s\n", dns_error_text(rcode));
        ui_reset();
        return;
    }
    ui_printf(";; ->>HEADER<<- opcode: QUERY, status: ");
    ui_color(rcode == DNS_OK || rcode == DNS_ERR_NODATA ? UI_GOOD : UI_BAD);
    ui_printf("%s", status_name(rcode));
    ui_reset();
    ui_printf(", id: %u\n;; flags:", r.id);
    for (unsigned i = 0; i < sizeof(flag_names) / sizeof(flag_names[0]); i++)
        if (r.flags & flag_names[i].bit)
            ui_printf(" %s", flag_names[i].name);
    ui_printf("; QUERY: 1, ANSWER: %d\n\n", r.count);

    ui_color(UI_TITLE);
    ui_printf(";; QUESTION SECTION:\n");
    ui_reset();
    ui_printf(";%s.\t\t\tIN\t%s\n", name, dns_type_name(type));
    if (r.count) {
        ui_color(UI_TITLE);
        ui_printf("\n;; ANSWER SECTION:\n");
        ui_reset();
        for (int i = 0; i < r.count; i++) {
            const dns_record_t *rec = &r.answers[i];
            ui_printf("%s.\t%u\tIN\t%s\t", rec->name, rec->ttl, dns_type_name(rec->type));
            ui_color(UI_ACCENT);
            print_data(rec);
            ui_reset();
            ui_printf("\n");
        }
    }
    ui_color(UI_DIM);
    ui_printf("\n;; Query time: %u msec\n;; SERVER: %s#53(%s)\n", r.time_ms, ip, ip);
    ui_reset();
}
