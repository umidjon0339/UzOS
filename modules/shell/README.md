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
│   ├── complete/  complete.h/.c    Tab: command names and paths
│   └── highlight/ highlight.h/.c   colour the command word while you type
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
   letter       line_insert (input_bound checks room), then redraw()
   Backspace    line_backspace, then redraw()
   Tab          complete_line: finish the word, or list the choices
   Up / Down    history_older / history_newer
   Ctrl+C       forget the typed line, print "^C", a new prompt
   Enter        history_add, then run():
                  "  cat   notes  "  ->  name "cat", args "notes"
                  shell_find("cat") -> cmd_cat("notes")
                ui_reset(): back to the normal text colour
                then the prompt "guest@uzos:~/path$ " again
```

## Colours while you type

After every key the line is drawn again in place (`redraw()`: one
backspace per character on the screen, then `highlight_print()`). Only
the command word gets a colour:

| You typed | Colour | Meaning |
|---|---|---|
| `ls` | green | a real command: Enter will run it |
| `l` | normal | not a command yet, but `ls` starts like this |
| `lx` | red | no command starts like this: a typo |

## Ctrl+C

Ctrl+C has to work even while a command runs and nobody reads the
keyboard queue. So the keyboard driver calls the shell's handler straight
from the interrupt:

```
Ctrl+C -> keyboard interrupt -> ctrl_c_pressed()
   a command is running  -> signal_send(shell, SIGINT)      (process_management)
                            commands that wait use sys_sleep_interruptible():
                            sleep, top, exec, snake, startx, password questions
                            stop early; afterwards run() prints "^C"
   at the prompt         -> KEY_CTRL_C goes to the queue: the line is cleared
```

`exec hello` runs in the **foreground**: the shell waits, and Ctrl+C
ends the program (through `process_kill`, so `kill_guard` still
decides). `exec hello &` runs it in the background, as before.

## The prompt

```
guest@uzos:~/docs$          a normal user, in /home/guest/docs
root@uzos:/home#            root: the prompt ends in '#'
```

Like a Linux shell: **who** (the logged-in user, from
`session_user()` in user_management), **which computer**, **where**
(the current directory, from `sys_getcwd`; your home directory is
shown as `~`). Root's prompt ends in `#` instead of `$`, so you always
see when you have every right. If the last output did not end with a
newline, the prompt starts on a fresh line.

Before the shell takes keys, `kernel_main` runs the login prompt
(user_management/core/session); `logout` brings it back.

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
