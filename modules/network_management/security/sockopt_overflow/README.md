# Attack: sockopt_overflow

| | |
|---|---|
| **Stopped by** | [`optlen_check`](defence/optlen_check/) |
| **Breaks** | integrity: the kernel reads an option only as far as the caller gave it |
| **Severity** | high |

## What it does

Option-length bugs: the kernel trusts a length and reads (or writes) past the caller's buffer.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains option lengths |
| `live` | sets three options with lengths that are too short |

All three are refused with EINVAL.

## Try it

```
seclab attack sockopt_overflow live
seclab verify optlen_check

```
