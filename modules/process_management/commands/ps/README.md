# 📘 UzOS Command Package: `ps` (Process Status)

Welcome to the **`ps` command package**! This directory contains the complete implementation and documentation for inspecting running processes inside **UzOS**.

---

## 🎯 1. Educational Objective
The `ps` command teaches one of the most fundamental concepts in modern operating systems:
> **How does an operating system track and monitor executing tasks?**

In operating system theory, a program in execution is called a **Process**. Every process is represented in kernel memory by a data structure called the **Process Control Block (PCB)**. In UzOS, this is `process_t`.

---

## 📂 2. Package Structure
```text
modules/process_management/commands/ps/
├── cmd_ps.h        # Package interface, command metadata & registration
├── cmd_ps.c        # Command implementation & table inspection logic
└── README.md       # This educational guide
```

---

## ⚙️ 3. How `ps` Works Step-by-Step

```text
User types 'ps' 
      │
      ▼
Shell calls cmd_ps()
      │
      ├── 1. Fetches Process Table: process_get_table()
      │
      ├── 2. Prints Header: [PID] [STATE] [NAME]
      │
      ├── 3. Loops through MAX_PROCESSES (64 slots):
      │      - Ignores PROCESS_STATE_FREE slots
      │      - Formats and displays active processes
      │
      └── 4. Returns control to shell prompt
```

---

## 🔄 4. Process States Explained

When you run `ps` in UzOS, each process will have one of these scheduler states:

| State | Constant | Description |
|---|---|---|
| **READY** | `PROCESS_STATE_READY` | Process is placed in the scheduler queue waiting for CPU time slice. |
| **RUNNING** | `PROCESS_STATE_RUNNING` | Process is actively executing instructions on the CPU core right now. |
| **SLEEPING** | `PROCESS_STATE_SLEEPING` | Process called `sleep()` and is waiting for timer ticks to elapse. |
| **ZOMBIE** | `PROCESS_STATE_ZOMBIE` | Process finished execution (`exit`) and is awaiting cleanup. |
| **FREE** | `PROCESS_STATE_FREE` | Unused slot in the table (ignored by `ps`). |

---

## 🧪 5. How to Test & Experiment in UzOS

1. **Launch the OS:**
   ```bash
   make run
   ```
2. **List running processes:**
   ```bash
   ps
   ```
   *You will see the Kernel/Shell process running.*

3. **Spawn a background worker thread:**
   ```bash
   spawn
   ```
4. **Run `ps` again:**
   ```bash
   ps
   ```
   *You will now see the newly spawned background process alongside the shell!*
