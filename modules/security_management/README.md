# security_management — protection, audit, integrity, and the security lab

An OS book splits this topic in two:

- **Protection**: who may do what. Is this user allowed to kill that
  process, read that file, change that password?
- **Security**: what happens when someone tries anyway. Was it refused,
  who tried, and is the kernel itself still the one that booted?

Every other module answers protection questions for its own objects
(file_access for files, kill_guard for processes, ...). This module
holds what they share:

| Question | Answered by |
|---|---|
| Which privileges does a user hold? | `core/caps`: one table of capabilities |
| What was refused, when, to whom? | `core/audit`: the audit log |
| Is the kernel's code unchanged? | `core/integrity`: SHA-256 of the code at boot vs now |
| Which attacks exist, and do the defences hold? | `core/registry`, `core/catalog`, `core/watchdog`: the security lab (`seclab`) |

```
security_management/
├── core/                         the machinery, one folder per part
│   ├── caps/       caps.h/.c          capabilities: CAP_KILL, CAP_SETUID, ...
│   ├── audit/      audit.h/.c         the audit log: a ring + a counter per defence
│   ├── integrity/  integrity.h/.c     measure .text and .rodata, compare later
│   ├── registry/   registry.h/.c      the list of attacks and defences
│   ├── catalog/    catalog.h/.c       registers every module's pairs at boot
│   └── watchdog/   watchdog.h/.c      notices a live attack that runs too long
├── commands/                     shell commands, one folder each
│   ├── security_commands.h/.c         the command table (group "Security")
│   └── seclab/ audit/ caps/ integrity/
├── syscalls/                     one folder per system call
│   └── getcaps/
└── security/                     attack/defence pairs about security itself
    ├── attack/   kernel_patch/  log_flood/
    └── defence/  integrity_check/  audit_limit/
```

What other modules hand to it, and what they ask of it:

| Module | Gives / asks |
|---|---|
| every module | its defences call `audit_log()` when they refuse; its pairs are listed in `core/catalog` |
| process_management | `process_t.uid` / `.pid` for the audit log; kill_guard and slot_quota ask `caps_has()`; the ELF loader asks `integrity_verify()` |
| user_management | user names for `audit` and `caps`; SHA-256 for `core/integrity`; setuid_guard and file_access ask `caps_has()`; its commands ask `caps_require()` |
| cpu_management | priority_bounds asks `caps_has()`; the TSC times `integrity` |
| device_management | `timer_ticks` stamps events; the timer interrupt calls `watchdog_tick()` |

## Capabilities

"Is this user root?" is a blunt question. A **capability** is one
privilege with a name. Every check asks for exactly the one it needs:

| Capability | Lets you | Asked by |
|---|---|---|
| `CAP_KILL` | stop processes of other users | kill_guard |
| `CAP_SETUID` | become another user, no password | setuid_guard, `su` |
| `CAP_FILES` | ignore file modes and owners | file_access |
| `CAP_CHOWN` | give files to another user | `chown` |
| `CAP_USERS` | add, remove, re-password users | `useradd`, `userdel`, `passwd <user>` |
| `CAP_PRIORITY` | raise priorities, renice others | priority_bounds |
| `CAP_RESOURCE` | no process quota, use the reserve | slot_quota |
| `CAP_SECLAB` | run live attacks and repair | `seclab` |

Who holds which is decided in **one** place, `caps_of()` in
[core/caps/caps.c](core/caps/caps.c): root holds all, every other user
none. Before, the same decision was a `uid == 0` test in a dozen places.
Now it can be read, listed (`caps`) and changed in one spot. This is
**least privilege**, the way Linux does it (`man 7 capabilities`).

```c
if (!caps_has(caller_uid, CAP_KILL)) ...        // in a rule: pure, uid in
if (!caps_require(CAP_USERS, "useradd")) ...    // in a command: prints why, logs it
```

Not every `UID_ROOT` is a privilege: the `#` in the prompt, root's red
colour, and "root's account cannot be deleted" are about *who* root
is, so they stay.

## The audit log

Every defence calls `audit_log(defence, detail)` when it refuses. The
log stamps the event with the time and the process (PID and uid)
that was running:

```
   41.27s  guest   PID 2     kill_guard
                             kill refused: system process or not yours
```

| Part | Keeps | When full |
|---|---|---|
| the ring | the newest 32 events, every detail | the oldest is overwritten (`overwritten`) |
| the counters | refusals per defence since boot | never: they only count |
| audit_limit | at most 10 events per defence per second go into the ring | the rest are only counted (`only counted`) |

- `audit_log()` is called from interrupt handlers too (the mouse, the
  timer, CPU faults), so it changes the ring with interrupts off.
- `audit_snapshot()` copies the ring the same way, so it cannot change
  while you print it.
- There is no "clear": a log that can be wiped is no proof.

The live verdict of `seclab` uses the counters: a run is **BLOCKED**
when the attack's *own* defence refused something during it. Other
defences may refuse too; that proves nothing about this one.

## Kernel integrity

```
boot:   at_boot = SHA-256(.text), SHA-256(.rodata)     integrity_init()
exec:   now     = SHA-256(...)                          integrity_verify()
        now == at_boot ?  yes: start the program
                          no:  refuse, log it           (defence integrity_check)
```

The linker marks where the regions start and end (`_text_start`,
`_rodata_end`, ... in [boot/linker.ld](../../boot/linker.ld)). `.rodata`
counts too: it holds the syscall table, which rootkits like to hook.
One check takes about 17 ms. It **detects**; it does not prevent:
UzOS maps its code writable (see Known limits).

## System calls

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 25 | `getcaps` | none | the caller's capabilities, one bit per `cap_t` (bit 0 = CAP_KILL) |

## Commands

| Command | What it does | Who |
|---|---|---|
| `audit` | the newest refusals: time, user, PID, defence, what | anyone |
| `audit sum` | refusals per defence since boot, and how many were only counted | anyone |
| `caps [user]` | which capabilities a user holds | anyone |
| `integrity` | hash `.text` and `.rodata` again, compare with boot | anyone |
| `seclab ...` | the security lab, see below | anyone; `live` and `repair` need CAP_SECLAB |

## The security lab

Every module shows how one of its guarantees can be **broken** (an
attack) and how the kernel **stops** it (a defence). The pairs live in
each module, next to the code they are about:
`modules/<module>/security/attack/<id>/` and `.../defence/<id>/`.

| Part | Must |
|---|---|
| `attack.simulate()` | only read; report what *would* happen. Always safe. |
| `attack.launch()` | really do it, but a small, fixed amount |
| `attack.cleanup()` | undo what `launch()` left (or `0` if nothing) |
| `defence.selftest()` | feed the rule bad inputs; return 1 if it holds |
| the defence check | be **called by the kernel** at the exact place the attack strikes, and call `audit_log()` when it refuses |

Each defence keeps its rule as a small **pure function** (numbers in,
yes/no out). The kernel calls it on real data; `selftest()` calls it on
made-up data. A defence that nothing calls is not a defence.

```
seclab list                  every attack -> its defence
seclab info <attack>         what it does, what stops it
seclab attack <id>           simulate (read-only)
seclab attack <id> live      run it for real (CAP_SECLAB)
seclab defence <id>          show a defence
seclab verify <id> | all     run selftests
seclab log                   the same as `audit`
seclab status                counters, watchdog, free process slots
seclab repair                every attack undoes itself (CAP_SECLAB)
```

- Some defences never need to refuse: the reaper cleans up, and the
  null page is set up at boot. With zero refusals, `seclab` falls back
  to that defence's selftest ("DEFENCE HOLDS").
- Some attacks leave processes behind (fork_bomb's workers count
  against root's quota), so run `seclab repair` between live runs.
- The **watchdog** gives each live run 2 seconds. If a run takes
  longer, the timer interrupt writes it to the audit log. It reports;
  it does not stop the attack.

### Adding a pair

1. `modules/<module>/security/attack/<id>/atk_<id>.c` with an `attack_t`
   and `atk_<id>_register()`, plus a README
2. `.../defence/<id>/def_<id>.h/.c` with the check the kernel calls, a
   `defence_t` and `def_<id>_register()`, plus a README
3. call the check from the kernel code it protects
4. declare and call both register functions in `core/catalog/catalog.c`

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `kernel_patch` | integrity | `integrity_check` | `integrity_verify` (`elf_load_and_run`, `integrity`) |
| `log_flood` | integrity | `audit_limit` | `audit_log` |

## Try it

```
uzos login: guest            (password guest)
caps                         # guest holds 0 of 8
useradd alice                # needs CAP_USERS
kill 1                       # kill_guard refuses
audit                        # both refusals, with user and PID
su                           (password uzos)
integrity                    # unchanged since boot
seclab attack kernel_patch live
seclab attack log_flood live
audit sum
seclab verify all            # 40 of 40 defences hold
```

## Known limits

- **Kernel code is writable.** The identity map gives the kernel's
  pages `PAGE_WRITABLE`, and `exec` programs run in ring 0, so
  integrity_check can only notice a patch, not stop it. Mapping
  `.text` read-only and setting CR0.WP is the next step.
- **Capabilities follow the uid.** A process cannot drop one
  capability and keep the rest; that needs a capability set per
  process (`process_t.caps`), like Linux has.
- **The audit log lives in RAM.** It is gone after a reboot.
