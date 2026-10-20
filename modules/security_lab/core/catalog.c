/*
 * catalog.c - The list of every attack and defence in UzOS.
 *
 * Each attack/defence folder has a <id>_register() function. To add a
 * new one: write the folder, declare its register function here, and
 * call it in proc_security_init(). The `seclab list` order follows this.
 */

#include "security_lab.h"

/* ---------- process_management ---------- */
void atk_fork_bomb_register(void);              void def_slot_quota_register(void);
void atk_kill_system_register(void);            void def_kill_guard_register(void);
void atk_stack_overflow_register(void);         void def_stack_guard_register(void);
void atk_zombie_leak_register(void);            void def_reaper_register(void);
void atk_pid_wrap_register(void);               void def_pid_bounds_register(void);
void atk_uninit_pcb_register(void);             void def_pcb_wipe_register(void);
void atk_kernel_stack_to_user_register(void);   void def_user_stack_register(void);
void atk_malicious_elf_register(void);          void def_elf_validate_register(void);

/* ---------- memory_management ---------- */
void atk_user_ptr_wrap_register(void);          void def_user_ptr_bounds_register(void);
void atk_mmap_length_wrap_register(void);       void def_mmap_page_count_register(void);
void atk_pmm_bad_free_register(void);           void def_pmm_free_guard_register(void);
void atk_heap_double_free_register(void);       void def_heap_guard_register(void);
void atk_heap_overflow_register(void);          void def_heap_canary_register(void);
void atk_munmap_kernel_register(void);          void def_mmap_ownership_register(void);
void atk_null_deref_register(void);             void def_null_page_register(void);

/* ---------- filesystem_management ---------- */
void atk_bad_filename_register(void);           void def_filename_check_register(void);
void atk_file_offset_wrap_register(void);       void def_file_range_register(void);
void atk_delete_open_file_register(void);       void def_busy_check_register(void);
void atk_dir_cycle_register(void);              void def_no_cycle_register(void);

/* ---------- device_management ---------- */
void atk_timer_divide_by_zero_register(void);   void def_timer_frequency_register(void);
void atk_mouse_coordinate_escape_register(void); void def_mouse_bounds_register(void);
void atk_mouse_desync_register(void);           void def_mouse_packet_sync_register(void);
void atk_irq_storm_register(void);              void def_irq_mask_register(void);
void atk_canvas_overflow_register(void);        void def_canvas_clip_register(void);
void atk_rtc_garbage_register(void);            void def_rtc_validate_register(void);
void atk_invisible_text_register(void);         void def_contrast_check_register(void);

/* ---------- io_management ---------- */
void atk_invalid_descriptor_register(void);     void def_descriptor_bounds_register(void);
void atk_oversized_transfer_register(void);     void def_transfer_bounds_register(void);
void atk_fd_leak_register(void);                void def_fd_cleanup_register(void);

/* ---------- cpu_management ---------- */
void atk_user_exception_register(void);         void def_fault_isolation_register(void);
void atk_tsc_timing_register(void);             void def_tsc_restrict_register(void);
void atk_priority_out_of_range_register(void);  void def_priority_bounds_register(void);
void atk_cpu_hog_register(void);                void def_aging_register(void);

/* ---------- shell ---------- */
void atk_calc_overflow_register(void);          void def_safe_arith_register(void);
void atk_line_overflow_register(void);          void def_input_bound_register(void);

/* ---------- ui ---------- */
void atk_output_spoof_register(void);           void def_safe_text_register(void);

void proc_security_init(void) {
    // process_management
    atk_fork_bomb_register();               def_slot_quota_register();
    atk_kill_system_register();             def_kill_guard_register();
    atk_stack_overflow_register();          def_stack_guard_register();
    atk_zombie_leak_register();             def_reaper_register();
    atk_pid_wrap_register();                def_pid_bounds_register();
    atk_uninit_pcb_register();              def_pcb_wipe_register();
    atk_kernel_stack_to_user_register();    def_user_stack_register();
    atk_malicious_elf_register();           def_elf_validate_register();

    // memory_management
    atk_user_ptr_wrap_register();           def_user_ptr_bounds_register();
    atk_mmap_length_wrap_register();        def_mmap_page_count_register();
    atk_pmm_bad_free_register();            def_pmm_free_guard_register();
    atk_heap_double_free_register();        def_heap_guard_register();
    atk_heap_overflow_register();           def_heap_canary_register();
    atk_munmap_kernel_register();           def_mmap_ownership_register();
    atk_null_deref_register();              def_null_page_register();

    // filesystem_management
    atk_bad_filename_register();            def_filename_check_register();
    atk_file_offset_wrap_register();        def_file_range_register();
    atk_delete_open_file_register();        def_busy_check_register();
    atk_dir_cycle_register();               def_no_cycle_register();

    // device_management
    atk_timer_divide_by_zero_register();    def_timer_frequency_register();
    atk_mouse_coordinate_escape_register(); def_mouse_bounds_register();
    atk_mouse_desync_register();            def_mouse_packet_sync_register();
    atk_irq_storm_register();               def_irq_mask_register();
    atk_canvas_overflow_register();         def_canvas_clip_register();
    atk_rtc_garbage_register();             def_rtc_validate_register();
    atk_invisible_text_register();          def_contrast_check_register();

    // io_management
    atk_invalid_descriptor_register();      def_descriptor_bounds_register();
    atk_oversized_transfer_register();      def_transfer_bounds_register();
    atk_fd_leak_register();                 def_fd_cleanup_register();

    // cpu_management
    atk_user_exception_register();          def_fault_isolation_register();
    atk_tsc_timing_register();              def_tsc_restrict_register();
    atk_priority_out_of_range_register();   def_priority_bounds_register();
    atk_cpu_hog_register();                 def_aging_register();

    // shell
    atk_calc_overflow_register();           def_safe_arith_register();
    atk_line_overflow_register();           def_input_bound_register();

    // ui
    atk_output_spoof_register();            def_safe_text_register();
}
