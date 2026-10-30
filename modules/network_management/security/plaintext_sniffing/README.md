# Attack: plaintext_sniffing (ILLUSTRATION)

| | |
|---|---|
| **Stopped by** | [`encryption`](defence/encryption/) |
| **Breaks** | disclosure: a password travels unreadable to anyone on the path |
| **Severity** | high |

## What it does

Plaintext protocols (telnet, FTP, HTTP, POP3) carry passwords as they were typed; a sniffer reads them.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains plaintext protocols |
| `live` | sends a USER/PASS login over lo and reads it off the tap |

The password is read off the wire. Nothing in UzOS stops it: this pair shows why TLS and SSH exist.

## Try it

```
seclab attack plaintext_sniffing live
seclab verify encryption
tcpdump -A port 23     # in another session: telnet
```
