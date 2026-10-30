# user_management — accounts, logging in, and who may use which file

Inside the kernel a user is only a **number**, the uid. Every process
has one (`process_t.uid`), every file has one (its owner). This module
gives those numbers names and passwords, decides who is logged in, and
holds the rule for who may read or change a file.

| Question | Answered by |
|---|---|
| Which users exist? | `core/users`: the user table (uid, name, home, salt, hash) |
| How is a password kept? | `core/hash`: SHA-256; only salt + hash are stored |
| Who is using the shell? | `core/session`: login prompt, `su`, `logout` |
| May this user read or change this file? | `core/perm`: owner + mode, CAP_FILES (root) passes |

```
user_management/
├── core/                         the machinery, one folder per part
│   ├── users/      users.h/.c         the user table, passwords, add/delete
│   ├── hash/       sha256.h/.c        SHA-256 (FIPS 180-4)
│   ├── session/    session.h/.c       login prompt, switching users, typed lines
│   └── perm/       perm.h/.c          permission checks, modes as text
├── commands/                     shell commands, one folder each
│   ├── user_commands.h/.c             the command table (group "Users")
│   └── whoami/ id/ users/ su/ logout/ passwd/ chmod/ chown/ useradd/ userdel/
├── syscalls/                     one folder per system call
│   └── getuid/ setuid/
└── security/                     attack/defence pairs (see security_management)
    ├── attack/   setuid_escalation/ file_permission_bypass/   (each with its .s program)
    └── defence/  setuid_guard/ file_access/
```

What other modules hand to it, and what they ask of it:

| Module | Gives / asks |
|---|---|
| process_management | `process_t.uid`; `process_caller_uid()` = who is asking; new processes get their creator's uid |
| filesystem_management | `vfs_node_t.owner` / `.mode`; its syscalls and `vfs_lookup` call `perm_check()` |
| io_management | `sys_open` calls `perm_check()` for read / write / create |
| shell | the prompt asks `session_user()` |
| security_management | `caps_has()` / `caps_require()` decide what root may do; `audit` and `caps` show user names |

## The users

| uid | name | password | home | |
|---|---|---|---|---|
| 0 | `root` | `uzos` | `/home/root` (mode 60, private) | holds every capability |
| 1 | `guest` | `guest` | `/home/guest` (mode 64) | a normal user |
| 1000+ | added with `useradd` | chosen then | `/home/<name>` | normal users |

A uid is **never reused**: after `userdel alice` (uid 1000), the next
user gets 1001. Otherwise the new user would quietly own every file
alice left behind. Those files show their owner as `#1000`.

## How a password is kept

```
salt = 16 fresh bytes                     (cycle counter + ticks, hashed)
hash = SHA-256(salt + password)
       then SHA-256(hash + salt), 1000 times in all

stored: salt and hash.  Not stored: the password.
login:  hash what was typed with the same salt, compare every byte
```

- The **salt** means two users with the same password still get
  different hashes.
- The **rounds** make each check slower: you don't notice it for one
  login, but it adds up for anyone trying many passwords.

`users` shows the start of each hash. There is nothing else to show.

## Logging in

```
boot -> "uzos login:"  name + hidden password  -> session_switch(user)
                                                   shell process uid = user's uid
                                                   cwd = user's home
su [user]  -> same switch; CAP_SETUID (root) needs no password
logout     -> back to "uzos login:"
```

The shell **is** a process (kernel_main), so "being logged in as guest"
just means the shell's uid is 1. Every check in the kernel then treats
the shell, and every process it starts, as guest. The prompt shows it:

```
guest@uzos:~$        a normal user, at home
root@uzos:/home#     root ends in '#'
```

## Permissions

Each file and directory has an **owner** and a **mode** of two octal
digits, owner first, then everybody else:

```
digit = 4 (read) + 2 (write)       64 -> rw/r-     60 -> rw/--     40 -> r-/--
```

| | read | write |
|---|---|---|
| file | `cat`, `cp` from, `exec` | `write`, `cp` to |
| directory | `ls`, `cd`, walking through it in a path | create or delete things in it |

- Deleting, renaming or `chmod` also needs you to **own** the entry.
- Only CAP_CHOWN (root) may use `chown`.
- CAP_FILES (root) passes every check.
- New entries belong to whoever created them, with mode 64.

One `open()`, step by step:

```
sys_open("/home/root/notes", O_READ)            io_management
  vfs_lookup: read on "/"? read on "home"? read on "root"?   (each directory)
  perm_check(notes, PERM_READ)                  core/perm -> file_access
  fd_alloc(...)
```

## System calls

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 23 | `getuid` | none | the caller's uid |
| 24 | `setuid` | EBX = uid | 0 / -1 (setuid_guard: only CAP_SETUID may change the uid); a real change makes the process non-dumpable (process_management defence dumpable_flag) |

## Commands

| Command | What it does | Who |
|---|---|---|
| `whoami` | the user this shell runs as | anyone |
| `id [user]` | uid, name, home | anyone |
| `users` | every account: uid, home, processes, hash | anyone |
| `su [user]` | become another user (default root) | password, or root |
| `logout` | back to the login prompt | anyone |
| `passwd [user]` | change your password; root may change anyone's | anyone / root |
| `chmod <mode> <path>` | who may read and write it | owner or root |
| `chown <user> <path>` | give it to another user | root |
| `useradd <name>` | new account with a home directory | root |
| `userdel <name>` | remove an account (files stay) | root |

Elsewhere: `ls` shows mode and owner, `ps`/`top` show user names, `cd`
alone goes home, and `seclab ... live` and `seclab repair` need
CAP_SECLAB.

## Root's rights are capabilities

"Root may do it" is never written as `uid == 0` in a check. Each check
asks for one capability (security_management/core/caps): CAP_SETUID
for `su` and setuid_guard, CAP_FILES for file_access, CAP_CHOWN for
`chown`, CAP_USERS for `useradd`, `userdel` and `passwd <user>`. Root
holds all of them, other users none; `caps [user]` lists them.

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `setuid_escalation` | privilege | `setuid_guard` | `sys_setuid` |
| `file_permission_bypass` | isolation | `file_access` | `perm_check` / `perm_may_remove` (fs and io syscalls, `vfs_lookup`, `ls`, ELF loader) |

## Try it

```
uzos login: guest            (password guest)
whoami; id; users
ls /home                     # root/ is rw/-- : guest cannot look in
write notes hello; chmod 60 notes; ls
su                           (password uzos)
useradd alice                # uid 1000, /home/alice
userdel alice; useradd bob   # bob gets 1001: uids are never reused
seclab attack file_permission_bypass live
logout
```
