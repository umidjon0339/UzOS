# shell — the command line

The shell reads what you type, finds the command, and runs it. It owns
no commands of other modules: every module registers its own at boot,
under a group name that `help` uses.

```
shell/
├── core/
│   ├── shell/     shell.h/.c       command registry, prompt, keys, run a line
│   ├── line/      line.h/.c        the line being typed (a safe fixed buffer)
│   ├── history/   history.h/.c     the last 16 commands, for Up / Down
│   └── complete/  complete.h/.c    Tab: command names and paths
├── commands/      shell_commands.h/.c + one folder per command:
│                  help clear echo color calc history snake
└── security/      attack/<id>/  defence/<id>/   (2 pairs, each with a README)
```

There is no `syscalls/` folder: a shell does not provide system calls,
it **uses** them (`chdir`, `getcwd`, `open`, ...). In Linux the shell is
an ordinary user program; in UzOS it still runs inside the kernel.

## From a key to a command

```
key pressed -> keyboard queue -> kernel_main loop -> shell_handle_key(c)
   letter       line_insert (input_bound checks room), echo it
   Backspace    line_backspace
   Tab          complete_line: finish the word, or list the choices
   Up / Down    history_older / history_newer
   Enter        history_add, then run():
                  "  cat   notes  "  ->  name "cat", args "notes"
                  shell_find("cat") -> cmd_cat("notes")
                ui_reset(): back to the normal text colour
                then the prompt "root@uzos:/path$ " again
```

## The prompt

```
root@uzos:/docs$
```

Like a Linux shell: **who** (UzOS has no user accounts yet, so the
shell is always root, UID 0), **which computer**, **where** (the
current directory, from `sys_getcwd`). If the last output did not end
with a newline, the prompt starts on a fresh line.

## Registering a command

```c
shell_begin_group("Files");                       // once per module
shell_register_command("ls", "List a directory", cmd_ls);
```

A command is a function `void cmd_x(const char *args)`. `args` is the
rest of the line, with the spaces around it already trimmed.

## The shell's own commands

| Command | What it does | Uses |
|---|---|---|
| `help [command]` | every command with a one-line description, by group; or one explained | the registry |
| `clear` | empty the screen (the status bar stays) | `vga_clear` |
| `echo <text>` | print the text | `vga_print_str` |
| `color <fg> [bg]` | the normal text colour, until changed again; unreadable pairs are refused | `ui_set_text_color`, `contrast_check` |
| `calc <expr>` | integer maths with `+ - * / %` and `( )` | `calc_eval` |
| `history` | the commands typed so far | core/history |
| `snake` | the game, centred on any screen size (Q quits, the old screen comes back) | `vga_fullscreen`, `vga_put_at`, speaker, keyboard |

`calc` is a **recursive descent** parser: one function per grammar rule
(expression → term → factor), so `*` binds tighter than `+` without any
special code.

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `calc_overflow` | liveness | `safe_arith` | `calc_eval`, every step |
| `line_overflow` | isolation | `input_bound` | `line_insert`, every key |
