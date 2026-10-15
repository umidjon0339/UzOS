# Device Management

```text
device_management/
├── keyboard/core/       # PS/2 keyboard driver
├── mouse/core/          # PS/2 mouse driver
├── timer/
│   ├── core/            # PIT timer driver
│   └── commands/        # uptime
├── pic/core/            # Programmable Interrupt Controller
├── vga_display/core/    # Text-mode display driver
├── gui/
│   ├── core/            # BGA, GUI, and window manager
│   └── commands/        # startx
└── security/
    ├── attack/          # Safe driver-policy probes
    └── defence/         # Shared policies called by drivers
```

Device drivers are part of Device Management: each driver keeps its
implementation in its own `core/`, with commands where applicable. The
shared security layer tests and protects those drivers.

## Security pairs

| Attack | Defence | Driver integration |
|---|---|---|
| `timer_divide_by_zero` | `timer_frequency` | PIT initialization rejects zero and frequencies that cannot fit its 16-bit divisor before port I/O. |
| `mouse_coordinate_escape` | `mouse_bounds` | Mouse IRQ packet updates use the shared screen-coordinate clamp. |

Try these from the system security lab:

```text
seclab attack timer_divide_by_zero
seclab attack timer_divide_by_zero live
seclab attack mouse_coordinate_escape
seclab verify timer_frequency
seclab verify mouse_bounds
```

The live timer probe submits only an invalid frequency; the defence refuses
it before touching PIT ports. Mouse probes test policy values without
changing live pointer state.
