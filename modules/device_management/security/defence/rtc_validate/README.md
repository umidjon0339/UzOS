# Defence: rtc_validate

| | |
|---|---|
| **Stops** | [`rtc_garbage`](../../attack/rtc_garbage/) |
| **Protects** | trust: data from hardware is checked before use |
| **Called from** | `rtc_decode()` in [rtc/](../../../rtc/rtc.c), after decoding |

## The rule

```
2000 <= year <= 2099      1 <= month <= 12      1 <= day <= days in that month
hour < 24                 minute < 60           second < 60
```

"Days in that month" knows leap years: 29 February 2024 passes,
29 February 2026 does not.

## Why

The RTC is a chip outside the kernel's control. With a dead battery, no
chip at all (every read gives `0xFF`) or a buggy emulator it can return
anything. Decoded as BCD, `0xFF` is 15 × 10 + 15 = **165**.

Code after the driver trusts the date:

| Code | What month 165 would do |
|---|---|
| `rtc_month_name(month)` | read entry 164 of a 12-entry table |
| `rtc_unix_time()` | `days_before[month - 1]`: the same, far past the table |
| the status bar, `date`, `uzfetch` | print nonsense |

So the driver checks the date **once**, in the one place every date
passes through, and hands out either a real date or nothing.

## Uses

- `rtc_time_t` from rtc/
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack rtc_garbage live
audit
date                              # still a real date
```
