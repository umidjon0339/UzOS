# Shell Module

## Overview
The Shell module provides the interactive command-line interface for uzos. It collects keyboard input, parses commands, and dispatches them to the correct module's registered handler.

## Architecture

```
modules/shell/
├── core/
│   └── shell.c              # Line buffer, command parser, dispatch engine
├── commands/
│   └── shell_commands.c     # Shell-owned commands: help, clear, echo
└── README.md
```

## Design Principle: Modular Command Registration
The shell does NOT contain commands from other modules. Instead, each module registers its own commands with the shell dispatcher during initialization:

- `modules/shell/commands/` → `help`, `clear`, `echo`
- `modules/memory_management/commands/` → `meminfo`, `heapinfo`
- `modules/device_management/timer/commands/` → `uptime`

This keeps each module self-contained and educational.

## API
- `shell_init()` — Initialize the shell line buffer
- `shell_register_command(name, desc, handler)` — Register a command from any module
- `shell_handle_key(c)` — Feed a keyboard character to the shell (called from keyboard IRQ)
