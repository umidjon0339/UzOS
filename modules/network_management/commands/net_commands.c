/*
 * net_commands.c - Register the network commands with the shell.
 *
 * One row per command: the name you type, the line `help` shows, and
 * the function in commands/<name>/cmd_<name>.c that runs it.
 *
 * Uses: shell_begin_group / shell_register_command (shell), and every
 *       cmd_<name>() of this module.
 */

#include "modules/network_management/commands/net_commands.h"     // this file
#include "modules/shell/core/shell/shell.h"                        // shell_register_command

// Hardware
#include "modules/network_management/commands/lspci/cmd_lspci.h"
#include "modules/network_management/commands/ethtool/cmd_ethtool.h"
// Interfaces, addresses, routes
#include "modules/network_management/commands/ip/cmd_ip.h"
#include "modules/network_management/commands/arp/cmd_arp.h"
#include "modules/network_management/commands/ifconfig/cmd_ifconfig.h"
#include "modules/network_management/commands/ifup/cmd_ifup.h"
#include "modules/network_management/commands/ifdown/cmd_ifdown.h"
// Watching
#include "modules/network_management/commands/netstat/cmd_netstat.h"
#include "modules/network_management/commands/ss/cmd_ss.h"
#include "modules/network_management/commands/tcpdump/cmd_tcpdump.h"
#include "modules/network_management/commands/iftop/cmd_iftop.h"
#include "modules/network_management/commands/nload/cmd_nload.h"
#include "modules/network_management/commands/nstat/cmd_nstat.h"
// Firewall
#include "modules/network_management/commands/iptables/cmd_iptables.h"
#include "modules/network_management/commands/nmap/cmd_nmap.h"
// Testing the syscalls
#include "modules/network_management/commands/netcalls/cmd_netcalls.h"
#include "modules/network_management/commands/route/cmd_route.h"
#include "modules/network_management/commands/dhclient/cmd_dhclient.h"
// Testing the link
#include "modules/network_management/commands/arping/cmd_arping.h"
#include "modules/network_management/commands/ping/cmd_ping.h"
#include "modules/network_management/commands/traceroute/cmd_traceroute.h"
#include "modules/network_management/commands/tracepath/cmd_tracepath.h"
#include "modules/network_management/commands/mtr/cmd_mtr.h"
// Names
#include "modules/network_management/commands/hostname/cmd_hostname.h"
#include "modules/network_management/commands/host/cmd_host.h"
#include "modules/network_management/commands/nslookup/cmd_nslookup.h"
#include "modules/network_management/commands/dig/cmd_dig.h"
// Clients and servers
#include "modules/network_management/commands/nc/cmd_nc.h"
#include "modules/network_management/commands/curl/cmd_curl.h"
#include "modules/network_management/commands/wget/cmd_wget.h"
#include "modules/network_management/commands/httpd/cmd_httpd.h"
#include "modules/network_management/commands/echod/cmd_echod.h"
#include "modules/network_management/commands/telnet/cmd_telnet.h"

static const struct {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} commands[] = {
    { "lspci",   "PCI devices (network card, ...): lspci [-n] [-v]", cmd_lspci },
    { "ethtool", "The network card: ethtool [-i | -S | -d | -t] [eth0]", cmd_ethtool },
    { "ip",      "Interfaces, addresses, routes: ip [-s] link|addr|route|neigh", cmd_ip },
    { "ifconfig", "Interfaces, classic: ifconfig [dev [addr] [netmask M] [up|down] ...]", cmd_ifconfig },
    { "ifup",     "Interface up + DHCP: ifup [eth0]", cmd_ifup },
    { "ifdown",   "Release DHCP + interface down: ifdown [eth0]", cmd_ifdown },
    { "netstat",  "Connections, ports, routes: netstat [-tuxlanp] | -r | -i | -s", cmd_netstat },
    { "ss",       "Socket statistics: ss [-tuxlanp] | ss -s", cmd_ss },
    { "tcpdump",  "Watch packets: tcpdump [-i dev|any] [-c N] [-A] [-X] [-e] [filter]", cmd_tcpdump },
    { "iftop",    "Busiest conversations now: iftop [-i dev] [-t sec]", cmd_iftop },
    { "nload",    "Interface speed: nload [dev] [-t sec]", cmd_nload },
    { "nstat",    "Network counters since last time: nstat [-a] [-z]", cmd_nstat },
    { "iptables", "Packet filter: iptables -L | -A|-I CHAIN rule -j T | -D | -P | -F", cmd_iptables },
    { "nmap",     "Port scanner: nmap [-sT|-sS|-sU|-sn] [-p ports] host|net/24", cmd_nmap },
    { "netcalls", "Test: ring 3 programs call the network syscalls [udp|tcp|local]", cmd_netcalls },
    { "arp",     "ARP cache, IP -> MAC: arp [-a] | -s IP MAC | -d IP", cmd_arp },
    { "route",   "Routing table: route [-n] | route add|del ...", cmd_route },
    { "dhclient", "Get an address by DHCP: dhclient [-r | -l] [eth0]", cmd_dhclient },
    { "arping",  "Who has this IP on my link? arping [-c N] [-I dev] IP", cmd_arping },
    { "ping",    "Is a host there? ping [-c N] [-s size] [-t ttl] [-i sec] host", cmd_ping },
    { "traceroute", "Routers on the way: traceroute [-m hops] [-q probes] host", cmd_traceroute },
    { "tracepath", "Path and its MTU: tracepath [-m hops] host", cmd_tracepath },
    { "mtr",      "Loss and delay of every hop: mtr [-c cycles] host", cmd_mtr },
    { "hostname", "This machine's name: hostname [-I | -d | newname]", cmd_hostname },
    { "host",     "DNS lookup: host [-t type] name | address", cmd_host },
    { "nslookup", "Ask a DNS server: nslookup name [server]", cmd_nslookup },
    { "dig",      "The whole DNS answer: dig [@server] name [type] [+short]", cmd_dig },
    { "nc",       "netcat: nc [-v] [-u] host port | -l [-u] port | -z host ports", cmd_nc },
    { "curl",     "Fetch a web page: curl [-I] [-v] http://host/path", cmd_curl },
    { "wget",     "Download a file: wget [-O file] http://host/path", cmd_wget },
    { "httpd",    "Web server in the background: httpd start [port] | stop | status", cmd_httpd },
    { "echod",    "Echo server in the background: echod start [port] | stop | status", cmd_echod },
    { "telnet",   "A text session with a server: telnet host [port]", cmd_telnet },
};

void register_net_commands(void) {
    shell_begin_group("Network");
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
        shell_register_command(commands[i].name, commands[i].description,
                               commands[i].handler);
}
