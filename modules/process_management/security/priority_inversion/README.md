# Attack: priority_inversion (concept)

| | |
|---|---|
| **Stopped by** | [`priority_inheritance`](defence/priority_inheritance/) |
| **Breaks** | liveness: a high-priority process must not wait for a medium one |
| **Severity** | info (concept) |

## What it does

LOW holds a lock, HIGH waits for it, MEDIUM needs no lock but outranks
LOW: LOW never runs, so HIGH waits for MEDIUM. This froze the Mars
Pathfinder lander in 1997.

## In UzOS

User programs share no locks (there is no futex), and the kernel turns
interrupts off instead of sleeping on locks, so the inversion cannot be
built. `live` shows what LOW runs at without and with the rule.

## Try it

```
seclab attack priority_inversion live
```
