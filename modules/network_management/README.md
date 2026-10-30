# network_management — from the network card to sockets

UzOS talks to the network: an Intel **e1000** driver, Ethernet, ARP,
IPv4 with fragments, ICMP, UDP, a full **TCP** (handshake, retransmit,
TIME_WAIT), DHCP, DNS, a firewall, and **sockets** for programs: UDP,
TCP, raw and AF_UNIX, with `select` / `poll` / `epoll`. It works in
QEMU and VirtualBox (both default to the e1000), and reaches the real
internet through QEMU's router.

```
network_management/
├── core/                         the stack (core/README.md: why each part is here)
│   ├── pci/        pci.h/.c           scan the PCI bus, find the card
│   ├── e1000/      e1000.h/.c         the card: registers, RX/TX rings, DMA, IRQ
│   │               eth0.c             join the driver to the stack as "eth0"
│   ├── netdev/     netdev.h/.c        interfaces, the receive backlog, the tap (tcpdump)
│   ├── loopback/   loopback.h/.c      lo: 127.0.0.1, sends straight back in
│   ├── packet/     packet.h/.c        packet buffers (like sk_buff): push / pull headers
│   ├── inet/       inet.h/.c          byte order, address text, checksums, NET_E* errors
│   ├── ethernet/   ethernet.h/.c      frames in and out
│   ├── arp/        arp.h/.c           IP -> MAC cache
│   ├── ipv4/       ipv4.h/.c          check, route, send   fragment.c  reassembly
│   ├── route/      route.h/.c         the routing table (longest prefix)
│   ├── icmp/       icmp.h/.c          ping, errors
│   ├── udp/        udp.h/.c           ports, endpoints
│   ├── tcp/        tcp.h/.c           the state machine   tcp_timer.c  retransmit, TIME_WAIT
│   ├── socket/     socket.h/.c        sockfs, waiting, options
│   │               sock_udp.c sock_tcp.c sock_raw.c   one file per kind
│   ├── unix/       unix.h/.c          AF_UNIX streams and datagrams
│   ├── poll/       poll.h/.c          wait for any of many descriptors
│   ├── epoll/      epoll.h/.c         the kernel-kept watch lists
│   ├── firewall/   firewall.h/.c      INPUT / OUTPUT rules (iptables)
│   ├── dhcp/       dhcp.h/.c          get an address at boot
│   ├── dns/        dns.h/.c           names -> addresses, /etc/hosts, a cache
│   ├── hostname/   hostname.h/.c      "uzos" (the prompt shows it)
│   └── net_tick/   net_tick.h/.c      every layer's timers, every 100 ms
├── commands/                     33 shell commands, one folder each (net_commands.c lists them)
├── syscalls/                     34 system calls, one folder each: sys_<name>.h/.c
└── security/                     47 attacks, each with its defence inside (security/README.md)
```

## The life of a packet

```
  a program: sendto(fd, "hi", 2, 0, &addr, len)                  int 0x80
      │  syscalls/sendto -> core/socket (sock_udp.c)
      ▼
  udp_send()      UDP header + checksum (pseudo header)       core/udp
  ipv4_send()     route lookup, IP header, fragments, OUTPUT   core/ipv4, route, firewall
  arp_output()    the next hop's MAC (or wait for the answer)  core/arp
  ethernet_send() Ethernet header                              core/ethernet
  netdev_transmit()  tcpdump's tap sees it                     core/netdev
  e1000_send()    into the TX ring; the card sends it (DMA)    core/e1000
      ▼
 ─────────────────────────────── wire ───────────────────────────────
      ▼
  e1000 IRQ       the frame is in an RX buffer (DMA)           core/e1000
  netdev_receive() copy into a packet, the backlog             core/netdev
  ethernet_receive() check, pull the header                    core/ethernet
  ipv4_receive()  check, believable source, for us?, fragments,
                  INPUT chain, raw sockets                     core/ipv4
  udp_receive()   find the port's endpoint                     core/udp
  socket_queue()  wait in the socket, wake the reader          core/socket
      ▲
  the program: recvfrom() returns
```

Everything inside the stack runs with interrupts off (the card's
interrupt, or a process that called `netdev_lock()`), so no layer is
ever seen half-way through a change. Received frames wait in one
backlog and go up one at a time: on `lo`, a reply to a packet is
handled after it, never inside it.

A socket operation that cannot finish yet (no data, no connection,
buffer full) returns `-EAGAIN` from the protocol; `socket.c` then puts
the process to sleep, and `socket_wake()` wakes it when something
arrives. Errors are Linux's numbers, negative (`-111` = ECONNREFUSED).

## TCP in one picture

```
 client                                     server
 connect()  SYN seq=x            ──────►    LISTEN -> SYN_RECV
            ◄──────  SYN+ACK seq=y ack=x+1
 ESTABLISHED  ACK ack=y+1        ──────►    ESTABLISHED   accept()
 send()       data seq=x+1       ──────►    recv()
            ◄──────  ACK
 close()      FIN                ──────►    CLOSE_WAIT    (recv() = 0)
 FIN_WAIT_2 ◄──────  ACK
            ◄──────  FIN                    close() -> LAST_ACK
 TIME_WAIT    ACK                ──────►    CLOSED
 (10 s) -> CLOSED
```

Lost segments are sent again after `rto_ms`, measured from the real
round-trip time (Jacobson / Karels) and doubled on every retry. Data
must arrive in order: a segment after a gap waits for its resend. A
full listen queue answers with SYN cookies; initial sequence numbers are
unguessable (RFC 6528); a RST must hit the exact next number (RFC 5961).

## At boot

```
[  OK  ] PCI bus          6 devices
[  OK  ] Network card     e1000, IRQ 11, MAC 52:54:00:12:34:56
[  OK  ] Interfaces       lo 127.0.0.1, eth0
[  OK  ] DHCP             eth0 10.0.2.15/24, gateway 10.0.2.2
```

`make run` gives QEMU `-nic user,model=e1000,hostfwd=tcp::8080-:80`:
UzOS is 10.0.2.15, the router 10.0.2.2, DNS 10.0.2.3, and port 8080 of
the host reaches port 80 of UzOS (`httpd start`, then
http://localhost:8080 in the host's browser).

## Commands

| Group | Commands |
|---|---|
| The card | `lspci`, `ethtool` (`-i -S -d -t`: the self-test sends a frame to itself) |
| Interfaces, addresses, routes | `ip link/addr/route/neigh`, `ifconfig`, `ifup`, `ifdown`, `route`, `arp` |
| Getting an address, names | `dhclient`, `hostname`, `host`, `nslookup`, `dig` |
| Is it there? | `arping`, `ping`, `traceroute`, `tracepath`, `mtr` |
| Clients and servers | `nc` (TCP/UDP, `-l`, `-z`), `telnet`, `curl`, `wget`, `httpd`, `echod` |
| Watching | `netstat`, `ss`, `tcpdump`, `iftop`, `nload`, `nstat`, `/proc/net/*` |
| Firewall and scanning | `iptables`, `nmap` (`-sT -sS -sU -sn`) |
| Testing the syscalls | `netcalls` (68 answers from ring 3) |

## System calls (numbers 133-166)

| Group | Calls |
|---|---|
| Sockets | `socket` 133, `bind` 134, `connect` 135, `listen` 136, `accept` 137, `accept4` 138, `getsockname` 139, `getpeername` 140, `socketpair` 141 |
| Data | `sendto` 142, `recvfrom` 143, `sendmsg` 144, `recvmsg` 145, `sendmmsg` 150, `recvmmsg` 151, `sendfile` 166 (and `read` / `write` / `close`) |
| Control | `shutdown` 146, `setsockopt` 147, `getsockopt` 148, `ioctl` 165 (FIONREAD, FIONBIO, SIOCGIF*/SIOCSIF*) |
| The old door | `socketcall` 149 |
| Waiting | `select` 152, `pselect6` 153, `poll` 154, `ppoll` 155, `epoll_create` 156, `epoll_create1` 157, `epoll_ctl` 158, `epoll_wait` 159, `epoll_pwait` 160 |
| Names | `uname` 161, `sethostname` 162, `setdomainname` 163, `gethostname` 164 |

`netcalls udp | tcp | local` runs a ring 3 program per group as user
guest and checks every answer (refusals included: a guest gets EPERM for
a raw socket and EACCES for port 53).

## Privileges

Three capabilities (security_management/core/caps), all root's:

| Capability | Needed for |
|---|---|
| `CAP_NET_ADMIN` | changing interfaces, addresses, routes, the ARP cache, the firewall, the MAC, the hostname |
| `CAP_NET_RAW` | raw sockets, promiscuous mode, `tcpdump`, `iftop`, `nmap -sS/-sU/-sn` |
| `CAP_NET_BIND` | binding a port below 1024 |

## Security

47 attack / defence pairs, one folder per attack with its defence inside
(`security/<attack>/defence/<defence>/`): 38 real, 3 illustrations, 6
concepts. Attacks put crafted frames into the receive path exactly as if
the card had received them (`netdev_inject`), so the defences meet them
in the real code. See [security/README.md](security/README.md).

```
seclab list
seclab attack syn_flood live
seclab verify all
```

## What UzOS leaves out

IPv6, TLS/SSH, routing between interfaces (UzOS is a host), TCP options
other than MSS (no window scaling, SACK or timestamps), out-of-order
reassembly in TCP, connection tracking in the firewall, network
namespaces, and multicast.
