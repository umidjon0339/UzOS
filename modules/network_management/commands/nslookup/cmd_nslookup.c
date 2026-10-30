/*
 * cmd_nslookup.c - `nslookup name [server]`: who answers, and what.
 *
 *   nslookup example.com
 *   Server:   10.0.2.3
 *   Address:  10.0.2.3#53
 *
 *   Non-authoritative answer:
 *   Name:     example.com
 *   Address:  93.184.215.14
 *
 * "Non-authoritative": the server we asked did not own the name; it
 * asked the owners for us (recursion) and passes their answer on.
 *
 * Uses: dns_query / dns_* (core/dns), inet_* (core/inet), arg_next,
 *       strcpy, ui_*.
 */

#include "modules/network_management/commands/nslookup/cmd_nslookup.h" // this command
#include "modules/network_management/core/dns/dns.h"                // dns_query, dns_nameserver
#include "modules/network_management/core/inet/inet.h"              // inet_parse, inet_format
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define FLAG_AA 0x0400              // "authoritative answer": the server owns the name

void cmd_nslookup(const char *args) {
    char name[DNS_NAME_MAX], server_text[20];
    const char *rest = arg_next(args, name, sizeof(name));
    arg_next(rest, server_text, sizeof(server_text));
    uint32_t server = 0;
    if (!name[0] || (server_text[0] && !inet_parse(server_text, &server))) {
        ui_usage("nslookup name [server]");
        return;
    }
    if (!server)
        server = dns_nameserver();

    char ip[INET_ADDR_TEXT];
    inet_format(server, ip);
    ui_printf("Server:   %s\n", ip);
    ui_printf("Address:  %s#53\n\n", ip);

    static dns_result_t result;
    int rcode = dns_query(name, DNS_TYPE_A, server, &result);
    if (rcode != DNS_OK) {
        ui_color(UI_WARNING);
        ui_printf("** server can't find %s: %s\n", name, dns_error_text(rcode));
        ui_reset();
        return;
    }
    if (!(result.flags & FLAG_AA))
        ui_printf("Non-authoritative answer:\n");
    for (int i = 0; i < result.count; i++) {
        const dns_record_t *r = &result.answers[i];
        if (r->type == DNS_TYPE_CNAME) {
            ui_printf("%s\tcanonical name = %s.\n", r->name, r->text);
        } else if (r->type == DNS_TYPE_A) {
            inet_format(r->ip, ip);
            ui_printf("Name:     %s\n", r->name);
            ui_printf("Address:  ");
            ui_color(UI_ACCENT);
            ui_printf("%s\n", ip);
            ui_reset();
        }
    }
}
