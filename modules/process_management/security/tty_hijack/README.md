# Attack: tty_hijack (concept)

| | |
|---|---|
| **Stopped by** | [`tty_guard`](defence/tty_guard/) |
| **Breaks** | integrity: only the person at the keyboard types commands |
| **Severity** | info (concept) |

## What it does

`su guest -c prog` runs prog on root's terminal. prog pushes
`chmod 777 /etc/shadow` and an Enter into the terminal's input
(`ioctl TIOCSTI`); when it exits, root's shell runs it, as root.

## In UzOS

There is one console and no input-injection call: only the keyboard
types. `live` shows the rule's answers.

## Try it

```
seclab attack tty_hijack
```
