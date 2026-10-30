# network_management/security — attacks and their defences

Every pair lives in its own folder named for the **attack**, with the
**defence** nested inside it (`security/<attack>/defence/<defence>/`),
the same shape as cpu_management and process_management.

Most attacks are **real**: they build the frames an attacker on the
link would send and hand them to the stack through `netdev_inject()`,
exactly as if the e1000 had received them; the defence then meets them
in the real receive path. Others set up real connections over `lo` and
attack those. **Illustrations** show a rule UzOS cannot be attacked
through for real (an honest card, no TLS); **concepts** belong to
switches, IOMMUs or namespaces UzOS does not have, and launch nothing.

```
seclab list                       every pair
seclab attack <attack> live       run one (seclab repair afterwards)
seclab verify all                 every defence's selftest
audit                             what the defences refused
```

Some live attacks need something running first: `slowloris` needs
`httpd start`, `dhcp_spoofing` a DHCP lease.


## Link layer (Ethernet, ARP, the card)

| Attack | Defence | Kind | What the defence does |
|---|---|---|---|
| [`arp_spoofing`](arp_spoofing/) | [`arp_guard`](arp_spoofing/defence/arp_guard/) | real | an ARP packet may not change a known MAC, nor add one nobody asked for |
| [`arp_flood`](arp_flood/) | [`arp_table_limit`](arp_flood/defence/arp_table_limit/) | real | at most 8 ARP entries may come from requests we did not ask for |
| [`mac_spoofing`](mac_spoofing/) | [`mac_lock`](mac_spoofing/defence/mac_lock/) | real | only CAP_NET_ADMIN may change an interface's MAC address |
| [`promiscuous_sniffing`](promiscuous_sniffing/) | [`promisc_cap`](promiscuous_sniffing/defence/promisc_cap/) | real | only CAP_NET_RAW may switch an interface to promiscuous mode |
| [`malformed_frame`](malformed_frame/) | [`frame_validate`](malformed_frame/defence/frame_validate/) | real | a frame must be 14-1514 bytes with a real EtherType |
| [`rx_ring_overflow`](rx_ring_overflow/) | [`rx_desc_check`](rx_ring_overflow/defence/rx_desc_check/) | illustration | the driver takes only whole, error-free frames that fit their buffer |
| [`mac_flooding`](mac_flooding/) | [`port_security`](mac_flooding/defence/port_security/) | concept | a switch port accepts only a few MACs (port security) |
| [`vlan_hopping`](vlan_hopping/) | [`vlan_pruning`](vlan_hopping/defence/vlan_pruning/) | concept | trunks carry only the VLANs they need, never the native one untagged |
| [`dma_attack`](dma_attack/) | [`iommu`](dma_attack/defence/iommu/) | concept | an IOMMU limits each device to the memory mapped for it |

## IPv4 and ICMP

| Attack | Defence | Kind | What the defence does |
|---|---|---|---|
| [`ip_spoofing`](ip_spoofing/) | [`rp_filter`](ip_spoofing/defence/rp_filter/) | real | a packet whose source would be routed out another interface is dropped |
| [`land_attack`](land_attack/) | [`martian_filter`](land_attack/defence/martian_filter/) | real | packets from impossible sources (0.0.0.0, broadcast, multicast, ourselves) are dropped |
| [`malformed_ip_header`](malformed_ip_header/) | [`ip_header_validate`](malformed_ip_header/defence/ip_header_validate/) | real | version 4, 20 <= IHL*4 <= total length <= packet, and a correct checksum |
| [`ping_of_death`](ping_of_death/) | [`reassembly_bounds`](ping_of_death/defence/reassembly_bounds/) | real | a fragment must end inside the reassembly buffer |
| [`teardrop`](teardrop/) | [`fragment_overlap_check`](teardrop/defence/fragment_overlap_check/) | real | a datagram whose fragments overlap is dropped whole |
| [`fragment_flood`](fragment_flood/) | [`frag_mem_limit`](fragment_flood/defence/frag_mem_limit/) | real | one source may hold at most 2 of the 4 reassembly slots |
| [`icmp_flood`](icmp_flood/) | [`icmp_ratelimit`](icmp_flood/defence/icmp_ratelimit/) | real | at most 10 ICMP replies and errors per 100 ms |
| [`smurf_attack`](smurf_attack/) | [`icmp_ignore_broadcast`](smurf_attack/defence/icmp_ignore_broadcast/) | real | an echo request sent to a broadcast address is never answered |
| [`icmp_redirect`](icmp_redirect/) | [`no_accept_redirects`](icmp_redirect/defence/no_accept_redirects/) | real | ICMP redirects never change the routing table |
| [`source_routing`](source_routing/) | [`drop_ip_options`](source_routing/defence/drop_ip_options/) | real | packets with source route options (LSRR, SSRR) are dropped |
| [`ip_id_prediction`](ip_id_prediction/) | [`random_ip_id`](ip_id_prediction/defence/random_ip_id/) | real | every IP id is unpredictable |
| [`ttl_recon`](ttl_recon/) | [`ttl_normalize`](ttl_recon/defence/ttl_normalize/) | illustration | the default TTL is the common 64 |

## TCP

| Attack | Defence | Kind | What the defence does |
|---|---|---|---|
| [`syn_flood`](syn_flood/) | [`syn_cookies`](syn_flood/defence/syn_cookies/) | real | a full listen queue answers with SYN cookies instead of dropping |
| [`tcp_seq_prediction`](tcp_seq_prediction/) | [`random_isn`](tcp_seq_prediction/defence/random_isn/) | real | every connection's ISN is a keyed hash of its addresses plus a clock |
| [`rst_injection`](rst_injection/) | [`rst_window_check`](rst_injection/defence/rst_window_check/) | real | a RST resets only at exactly the next expected sequence number |
| [`tcp_session_hijack`](tcp_session_hijack/) | [`seq_window_check`](tcp_session_hijack/defence/seq_window_check/) | real | a segment must lie in the receive window, and data must be next in line |
| [`xmas_null_scan`](xmas_null_scan/) | [`tcp_flag_sanity`](xmas_null_scan/defence/tcp_flag_sanity/) | real | segments with impossible flag combinations are dropped silently |
| [`port_scan`](port_scan/) | [`scan_detect`](port_scan/defence/scan_detect/) | real | a source that hits 10 closed ports in a second gets no more RSTs for 10 s |
| [`slowloris`](slowloris/) | [`idle_timeout`](slowloris/defence/idle_timeout/) | real | a request must be complete within 5 s in total, not per read |
| [`zero_window`](zero_window/) | [`persist_timeout`](zero_window/defence/persist_timeout/) | real | a window that stays 0 for 4 probes ends the connection |
| [`sack_panic`](sack_panic/) | [`mss_min`](sack_panic/defence/mss_min/) | real | a peer's MSS below 48 is raised to 48 |
| [`connection_exhaustion`](connection_exhaustion/) | [`conn_quota`](connection_exhaustion/defence/conn_quota/) | real | a normal user may own at most 8 TCP connections at once |
| [`time_wait_assassination`](time_wait_assassination/) | [`rfc1337`](time_wait_assassination/defence/rfc1337/) | real | a RST never ends TIME_WAIT early |

## UDP and services

| Attack | Defence | Kind | What the defence does |
|---|---|---|---|
| [`udp_flood`](udp_flood/) | [`udp_ratelimit`](udp_flood/defence/udp_ratelimit/) | real | at most 5 'port unreachable' answers per second to one source |
| [`dns_spoofing`](dns_spoofing/) | [`dns_txid_port_random`](dns_spoofing/defence/dns_txid_port_random/) | real | every DNS query uses a random id and a random source port |
| [`dhcp_spoofing`](dhcp_spoofing/) | [`dhcp_validate`](dhcp_spoofing/defence/dhcp_validate/) | real | an offer must be self-consistent, and once we had a server, from that server |
| [`dhcp_starvation`](dhcp_starvation/) | [`dhcp_snooping`](dhcp_starvation/defence/dhcp_snooping/) | concept | the switch limits DHCP requests per port and checks the MAC inside them |
| [`amplification_reflection`](amplification_reflection/) | [`response_limit`](amplification_reflection/defence/response_limit/) | concept | a server limits answers per client and never answers much bigger than asked |
| [`plaintext_sniffing`](plaintext_sniffing/) | [`encryption`](plaintext_sniffing/defence/encryption/) | illustration | secrets travel only inside an encrypted channel (TLS, SSH) |

## Sockets and the kernel

| Attack | Defence | Kind | What the defence does |
|---|---|---|---|
| [`privileged_port_bind`](privileged_port_bind/) | [`port_privilege`](privileged_port_bind/defence/port_privilege/) | real | binding a port below 1024 needs CAP_NET_BIND |
| [`raw_socket_abuse`](raw_socket_abuse/) | [`raw_cap`](raw_socket_abuse/defence/raw_cap/) | real | SOCK_RAW needs CAP_NET_RAW |
| [`port_hijack`](port_hijack/) | [`reuseport_owner`](port_hijack/defence/reuseport_owner/) | real | SO_REUSEPORT sockets may share a port only with the same user's |
| [`sockbuf_exhaustion`](sockbuf_exhaustion/) | [`rmem_limit`](sockbuf_exhaustion/defence/rmem_limit/) | real | a socket's queue is charged a whole buffer per datagram, up to its rcvbuf |
| [`sockopt_overflow`](sockopt_overflow/) | [`optlen_check`](sockopt_overflow/defence/optlen_check/) | real | an option's length must be at least the size of the option |
| [`sockaddr_leak`](sockaddr_leak/) | [`sockaddr_zero`](sockaddr_leak/defence/sockaddr_zero/) | real | every address handed to a program is wiped before it is filled in |
| [`unix_impersonation`](unix_impersonation/) | [`peercred_check`](unix_impersonation/defence/peercred_check/) | real | SO_PEERCRED reports the connecting socket's owner, recorded by the kernel at connect() |
| [`firewall_bypass_fragments`](firewall_bypass_fragments/) | [`reassemble_before_filter`](firewall_bypass_fragments/defence/reassemble_before_filter/) | real | the firewall sees only whole datagrams: fragments are reassembled first |
| [`netns_escape`](netns_escape/) | [`namespaces`](netns_escape/defence/namespaces/) | concept | each container gets its own network namespace, and never the host's capabilities |

38 real, 3 illustrations, 6 concepts: 47 pairs.


## Bugs the pairs found while they were built

- `arp_spoofing`: `arp_request()` asked without remembering it, so
  `arp_guard` threw the real answer away and a forged request could
  create the gateway's entry. Asking now records an INCOMPLETE entry.
- `syn_flood`: forged sources under QEMU's router answered every
  SYN-ACK with RST, so the queue never filled: the attack now forges
  addresses nobody on the link has (their SYN-ACKs die in ARP).
- `slowloris`: data that arrived for a connection its program had
  already closed was kept; TCP now answers it with RST (RFC 1122).
- `tcp_seq_prediction` and friends: a server port held by a
  connection in TIME_WAIT could not be bound again; servers set
  SO_REUSEADDR.
