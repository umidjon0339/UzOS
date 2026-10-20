# Defence: input_bound

| | |
|---|---|
| **Stops** | [`line_overflow`](../../attack/line_overflow/) |
| **Protects** | isolation: typed text stays inside its buffer |
| **Called from** | `line_insert()` in [core/line](../../../core/line/line.c), for every key |

## The rule

```
used + 1 < capacity        (the last slot is kept for the final '\0')
```

When the line is full, extra keys are dropped. The first dropped key is
logged once per line, not every one.

## Why

The line being typed is a fixed array of 256 bytes. A keyboard can
deliver any number of keys. Without the check, key number 257 is stored
**past the end** of the array, over whatever lies next to it in memory.
That is a *buffer overflow*: the bug behind the Morris worm and many
other famous attacks.

All writes to the line go through one function, `line_insert()`, so
there is exactly one place where the check is needed.

## Uses

- `sec_log()` from the security lab

## Try it

```
seclab attack line_overflow live
```
