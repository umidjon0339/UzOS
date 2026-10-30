# network_management/core — the network stack

`core/` holds only what the kernel itself runs (an interrupt, the timer,
a syscall's inner work), or state that must exist exactly once. Anything
only one command uses lives in that command's folder (the PCI name
table is in `commands/lspci/`, the HTTP code in `commands/curl/` and
`commands/wget/`, each with its own copy).

| Part | Why it is in core |
|---|---|
| `pci/` | kernel_main scans the bus at boot; the e1000 driver looks itself up |
| `e1000/` | the card's interrupt; `eth0.c` registers it with netdev |
| `netdev/` | every layer sends and receives through it; one interface table |
| `loopback/` | the `lo` interface: one |
| `packet/` | one pool of buffers for everyone |
| `inet/` | byte order, checksums and errors every layer needs |
| `ethernet/`, `arp/`, `ipv4/`, `route/`, `icmp/`, `udp/`, `tcp/` | the protocols, called from the receive path |
| `socket/`, `unix/` | the socket table (one), called by every socket syscall |
| `poll/`, `epoll/` | the waiting shared by five syscalls; the epoll instances (state) |
| `firewall/` | the rule table (one), asked for every packet |
| `dhcp/` | kernel_main asks for an address at boot; the lease (one) |
| `dns/` | the resolver's cache (one) and /etc/hosts |
| `hostname/` | the host name (one): the prompt and uname read it |
| `net_tick/` | the timer interrupt runs every layer's timers through it |

Defences are not in core: each lives with its attack in
`security/<attack>/defence/<defence>/`, and the core file includes it
and calls it at the exact place (search for `DEFENCE`).

## Locking

The stack runs with interrupts off: in the card's interrupt, in the
timer interrupt (`net_tick`), and wherever a process enters it through
`netdev_lock()` / `netdev_unlock()` (the socket layer does this around
every protocol call). A process never sleeps with the lock held:
`socket.c` drops it, naps, and tries again.

## Two rules learned the hard way

- Update the connection's state BEFORE sending (`tcp_output`): on `lo`
  the answer arrives inside the send call, and its ACK must already make
  sense. The first version advanced `snd_nxt` after the send; both ends
  then disagreed and bounced ACKs forever with interrupts off.
- Answers to unusable segments (challenge ACKs) are rate-limited, so
  two confused ends can never loop.
