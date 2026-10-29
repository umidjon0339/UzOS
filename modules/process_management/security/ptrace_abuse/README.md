# ptrace_abuse -> ptrace_restrict

**Attack.** Use a debugging interface (`ptrace`) to attach to a process
you don't own and read its registers and memory - keys, passwords,
tokens.

**Defence (ptrace_restrict).** UzOS has no `ptrace()` yet; this is the
gate one would need: a system process is off limits, a debug-capable
user may attach to any other, and otherwise you may only debug a process
you own (as Linux does with Yama's `ptrace_scope`).
