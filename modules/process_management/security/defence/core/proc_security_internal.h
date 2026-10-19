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

void atk_user_ptr_wrap_register(void);
void atk_mmap_length_wrap_register(void);
void atk_pmm_free_oob_register(void);

void def_user_ptr_bounds_register(void);
void def_mmap_page_count_register(void);
void def_pmm_free_bounds_register(void);

void atk_path_traversal_register(void);
void atk_file_offset_wrap_register(void);
void def_path_component_register(void);
void def_file_range_register(void);

void atk_timer_divide_by_zero_register(void);
void atk_mouse_coordinate_escape_register(void);
void def_timer_frequency_register(void);
void def_mouse_bounds_register(void);

void atk_invalid_descriptor_register(void);
void def_descriptor_bounds_register(void);
void atk_oversized_transfer_register(void);
void def_transfer_bounds_register(void);

#endif
