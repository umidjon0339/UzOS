# Defence: tty_guard (concept)

| | |
|---|---|
| **Stops** | [`tty_hijack`](../../) |
| **Protects** | integrity: a terminal's input |
| **Called from** | nothing yet: UzOS has no input-injection call |

## The rule

Only a process of the session that owns a terminal may push input into
it (`def_tty_inject_ok`); never another session. Linux restricted
`TIOCSTI` (`dev.tty.legacy_tiocsti = 0`); `setsid` per `su` also takes
the terminal away.
