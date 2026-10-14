#ifndef PROC_SECURITY_INTERNAL_H
#define PROC_SECURITY_INTERNAL_H

// Core attack registration functions
void atk_fork_bomb_register(void);
void atk_kill_init_register(void);
void atk_stack_overflow_register(void);
void atk_zombie_leak_register(void);
void atk_pid_wrap_register(void);
void atk_uninit_pcb_register(void);

// Core defence registration functions
void def_slot_quota_register(void);
void def_kill_guard_register(void);
void def_stack_guard_register(void);
void def_reaper_register(void);
void def_pid_bounds_register(void);
void def_pcb_wipe_register(void);

#endif
