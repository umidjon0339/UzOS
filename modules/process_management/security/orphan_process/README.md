# orphan_process -> orphan_reaping

**Attack.** Let a parent exit before its children. Those orphans, on a
system that reaps a zombie only when its parent calls `wait()`, could sit
dead forever and leak process slots.

**Defence (orphan_reaping).** Linux re-parents orphans to init (pid 1);
UzOS uses a **global reaper** that collects every zombie by its state,
parent alive or not - same result, no leak. (See also zombie_leak, the
same reaper from the "parent never reaped me" angle.)
