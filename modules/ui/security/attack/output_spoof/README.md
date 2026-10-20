# Attack: output_spoof

| | |
|---|---|
| **Stopped by** | [`safe_text`](../../defence/safe_text/) |
| **Breaks** | consistency: a table shows exactly what is in the kernel |
| **Severity** | medium |

## What it does

It starts a process whose **name** contains a newline and a fake row:

```
"x\n      1  RUNNING    0  init"
```

Printed raw, `ps` would show:

```
      7  READY      0  x
      1  RUNNING    0  init      <- does not exist
```

The user now believes in a process that is not there. The same trick
can hide one that is (overwrite its row with `\r`).

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows how the raw name would print |
| `live` | starts a real process with that name, runs the real `ps`, then stops the process |

With the defence, the row shows `x?      1  RUNNING »`: one row, the
newline as `?`, the long name cut.

## Uses

`process_create()` / `process_kill()` (process_management), `cmd_ps()`.

## Try it

```
seclab attack output_spoof live
```
