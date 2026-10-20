# security_lab — attacks and defences, side by side

Every module shows how one of its guarantees can be **broken** (an
attack) and how the kernel **stops** it (a defence). This module is the
shared part: it keeps the list of attacks and defences, logs every
refusal, and gives you the `seclab` command.

```
security_lab/
├── core/
│   ├── security_lab.h/.c   registry, event log, watchdog
│   └── catalog.c           the list of every attack/defence in UzOS
└── commands/
    ├── security_lab_commands.h
    └── seclab/cmd_seclab.c the `seclab` shell command
```

The attacks and defences themselves live in each module, next to the
code they are about: `modules/<module>/security/attack/<id>/` and
`.../defence/<id>/`.

## The contract

| Part | Must |
|---|---|
| `attack.simulate()` | only read; report what *would* happen. Always safe. |
| `attack.launch()` | really do it, but a small, fixed amount |
| `attack.cleanup()` | undo what `launch()` left (or `0` if nothing) |
| `defence.selftest()` | feed the rule bad inputs; return 1 if it holds |
| the defence check | be **called by the kernel** at the exact place the attack strikes, and call `sec_log()` when it refuses |

Each defence keeps its rule as a small **pure function** (numbers in,
yes/no out). The kernel calls it on real data; `selftest()` calls it on
made-up data. A defence that nothing calls is not a defence.

## The `seclab` command

```
seclab list                  every attack -> its defence
seclab info <attack>         what it does, what stops it
seclab attack <id>           simulate (read-only)
seclab attack <id> live      run it for real; counts refusals (root only)
seclab defence <id>          show a defence
seclab verify <id> | all     run selftests
seclab log                   newest refusals first
seclab status                counters, watchdog, free process slots
seclab repair                every attack undoes itself (root only)
```

Live runs and `repair` start and kill processes of every user, so they
need root (`su` first). Everything else only reads and is open to all.

A live run is **BLOCKED** if any defence refused something during it.
Some defences never need to refuse (the reaper cleans up, the null page
is set up at boot), so with zero refusals `seclab` falls back to that
defence's selftest.

The **watchdog** gives each live run 2 seconds. If the run takes longer,
the timer interrupt notes it in the log. It reports; it does not stop
the attack.

## Adding a pair

1. `modules/<module>/security/attack/<id>/atk_<id>.c` with an `attack_t`
   and `atk_<id>_register()`, plus a README
2. `.../defence/<id>/def_<id>.h/.c` with the check the kernel calls, a
   `defence_t` and `def_<id>_register()`, plus a README
3. call the check from the kernel code it protects
4. declare and call both register functions in `core/catalog.c`

## Uses

`timer_ticks` (device_management/timer) to time-stamp events,
`process_count_free_slots()` and the reaper (process_management) for
`status` and `repair`.

API names still carry the old `proc_security_` / `sec_` prefix so the
other modules did not need changes. They will be renamed when those
modules are cleaned.
