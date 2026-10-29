# window_exhaustion -> window_quota

**Attack.** Open window after window. The window manager has MAX_WINDOWS
fixed slots, each with a heap canvas; without a cap a program takes them
all and nothing else can open one.

**Defence (window_quota).** `wm_create_window()` asks `def_window_quota_ok`
before taking a slot and refuses once all are in use. The attack opens
MAX_WINDOWS + 4 tiny windows and sees the extras refused, then destroys
the ones it made.
