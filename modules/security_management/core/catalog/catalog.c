/*
 * catalog.c - The list of every attack and defence in UzOS.
 *
 * Each attack/defence folder has a <id>_register() function, which hands
 * its attack_t / defence_t to the registry (core/registry). To add a new
 * one: write the folder, declare its register function here, and call it
 * in catalog_init(). The `seclab list` order follows this.
 */

#include "catalog.h"

/* ---------- process_management ---------- */
void atk_fork_bomb_register(void);              void def_slot_quota_register(void);
void atk_kill_system_register(void);            void def_kill_guard_register(void);
void atk_stack_overflow_register(void);         void def_stack_guard_register(void);
void atk_zombie_leak_register(void);            void def_reaper_register(void);
void atk_pid_wrap_register(void);               void def_pid_bounds_register(void);
void atk_uninit_pcb_register(void);             void def_pcb_wipe_register(void);
void atk_kernel_stack_to_user_register(void);   void def_user_stack_register(void);
void atk_malicious_elf_register(void);          void def_elf_validate_register(void);
/* process_management - new process attacks (signals / sessions / isolation) */
void atk_signal_abuse_register(void);           void def_signal_permission_register(void);
void atk_ptrace_abuse_register(void);           void def_ptrace_restrict_register(void);
void atk_orphan_process_register(void);         void def_orphan_reaping_register(void);
void atk_toctou_race_register(void);            void def_atomic_check_register(void);
void atk_process_injection_register(void);      void def_process_isolation_register(void);
void atk_process_hijacking_register(void);      void def_control_flow_integrity_register(void);
void atk_container_escape_register(void);       void def_namespace_isolation_register(void);
void atk_process_dos_register(void);            void def_service_limits_register(void);
/* process_management - signals with handlers (phase 1) */
void atk_signal_flood_register(void);           void def_sigqueue_limit_register(void);
void atk_sigreturn_forgery_register(void);      void def_sigframe_check_register(void);
void atk_handler_hijack_register(void);         void def_handler_bounds_register(void);
/* process_management - looking at processes (phase 2) */
void atk_process_masquerade_register(void);     void def_name_guard_register(void);
void atk_proc_info_leak_register(void);         void def_hidepid_register(void);
void atk_kernel_pointer_leak_register(void);    void def_kptr_hide_register(void);
/* process_management - waiting and handles (phase 3) */
void atk_pid_reuse_race_register(void);         void def_pidfd_handle_register(void);
void atk_wait_steal_register(void);             void def_child_only_register(void);
void atk_use_after_exit_register(void);         void def_pcb_generation_register(void);
/* process_management - jobs and sessions (phase 4) */
void atk_pgid_hijack_register(void);            void def_session_check_register(void);
/* process_management - sleeping and timers (phase 5) */
void atk_sleep_overflow_register(void);         void def_sleep_bounds_register(void);
void atk_timer_flood_register(void);            void def_timer_limits_register(void);
/* process_management - sandboxes (phase 6) */
void atk_syscall_abuse_register(void);          void def_seccomp_filter_register(void);
void atk_no_new_privs_bypass_register(void);    void def_no_new_privs_register(void);
/* process_management - debugging (phase 7) */
void atk_memory_peek_register(void);            void def_vm_access_check_register(void);
/* process_management - execve (phase 8) */
void atk_argv_overflow_register(void);          void def_arg_limits_register(void);
void atk_env_injection_register(void);          void def_env_sanitize_register(void);
void atk_fd_leak_on_exec_register(void);        void def_close_on_exec_register(void);
/* process_management - the last five (phase 9) */
void atk_thread_exhaustion_register(void);      void def_thread_quota_register(void);
void atk_stack_clash_register(void);            void def_guard_gap_register(void);
void atk_core_dump_leak_register(void);         void def_dumpable_flag_register(void);
void atk_priority_inversion_register(void);     void def_priority_inheritance_register(void);
void atk_tty_hijack_register(void);             void def_tty_guard_register(void);

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
/* filesystem_management - metadata (phase 1) */
void atk_umask_weak_register(void);             void def_secure_umask_register(void);
void atk_huge_file_register(void);              void def_size_cap_register(void);
void atk_timestamp_forgery_register(void);      void def_time_owner_register(void);
/* filesystem_management - directories (phase 2) */
void atk_dirent_leak_register(void);            void def_dir_read_check_register(void);
void atk_long_path_register(void);              void def_path_length_register(void);
void atk_deep_nesting_register(void);           void def_depth_limit_register(void);
void atk_path_walk_bypass_register(void);       void def_search_check_register(void);
/* filesystem_management - links (phase 3) */
void atk_symlink_race_register(void);           void def_protected_symlinks_register(void);
void atk_symlink_loop_register(void);           void def_link_depth_register(void);
void atk_hardlink_abuse_register(void);         void def_protected_hardlinks_register(void);
void atk_nlink_overflow_register(void);         void def_nlink_bounds_register(void);
void atk_use_after_unlink_register(void);       void def_inode_refcount_register(void);

/* ---------- device_management ---------- */
void atk_timer_divide_by_zero_register(void);   void def_timer_frequency_register(void);
void atk_mouse_coordinate_escape_register(void); void def_mouse_bounds_register(void);
void atk_mouse_desync_register(void);           void def_mouse_packet_sync_register(void);
void atk_irq_storm_register(void);              void def_irq_mask_register(void);
void atk_canvas_overflow_register(void);        void def_canvas_clip_register(void);
void atk_rtc_garbage_register(void);            void def_rtc_validate_register(void);
void atk_invisible_text_register(void);         void def_contrast_check_register(void);
/* device_management - new device attacks */
void atk_keyboard_flood_register(void);         void def_key_queue_bound_register(void);
void atk_speaker_abuse_register(void);          void def_speaker_freq_guard_register(void);
void atk_bga_mode_overflow_register(void);      void def_mode_bounds_register(void);
void atk_window_exhaustion_register(void);      void def_window_quota_register(void);
void atk_window_size_overflow_register(void);   void def_window_bounds_register(void);

/* ---------- io_management ---------- */
void atk_invalid_descriptor_register(void);     void def_descriptor_bounds_register(void);
void atk_oversized_transfer_register(void);     void def_transfer_bounds_register(void);
void atk_fd_leak_register(void);                void def_fd_cleanup_register(void);

/* ---------- cpu_management ---------- */
void atk_user_exception_register(void);         void def_fault_isolation_register(void);
void atk_side_channel_register(void);             void def_tsc_restrict_register(void);
void atk_priority_abuse_register(void);  void def_priority_bounds_register(void);
void atk_starvation_register(void);                void def_aging_register(void);

/* cpu_management - new CPU attacks (resource / scheduling / privilege) */
void atk_cpu_exhaustion_register(void);           void def_cpu_time_limit_register(void);
void atk_infinite_loop_register(void);            void def_cpu_watchdog_register(void);
void atk_resource_exhaustion_register(void);      void def_resource_quota_register(void);
void atk_cpu_fork_bomb_register(void);            void def_process_quota_register(void);
void atk_denial_of_service_register(void);        void def_fair_scheduling_register(void);
void atk_scheduling_manipulation_register(void);  void def_policy_restriction_register(void);
void atk_realtime_scheduling_abuse_register(void);void def_rt_throttling_register(void);
void atk_privilege_escalation_register(void);     void def_permission_check_register(void);
/* cpu_management - microarchitecture (illustrations) */
void atk_spectre_register(void);                  void def_user_kernel_isolation_register(void);
void atk_meltdown_register(void);                 void def_page_separation_register(void);
void atk_cache_timing_register(void);             void def_coarse_timer_register(void);
void atk_branch_prediction_register(void);        void def_speculation_barrier_register(void);
/* cpu_management - concepts (documented, not reproducible here) */
void atk_distributed_denial_of_service_register(void); void def_rate_limiting_register(void);
void atk_rowhammer_register(void);                void def_ecc_and_refresh_register(void);
void atk_return_oriented_programming_register(void);   void def_wx_and_aslr_register(void);
void atk_code_injection_register(void);           void def_nx_pages_register(void);
void atk_hypervisor_escape_register(void);        void def_vm_isolation_register(void);

/* ---------- user_management ---------- */
void atk_setuid_escalation_register(void);      void def_setuid_guard_register(void);
void atk_file_permission_bypass_register(void); void def_file_access_register(void);

/* ---------- shell ---------- */
void atk_calc_overflow_register(void);          void def_safe_arith_register(void);
void atk_line_overflow_register(void);          void def_input_bound_register(void);

/* ---------- ui ---------- */
void atk_output_spoof_register(void);           void def_safe_text_register(void);

/* ---------- security_management ---------- */
void atk_kernel_patch_register(void);           void def_integrity_check_register(void);
void atk_log_flood_register(void);              void def_audit_limit_register(void);

void catalog_init(void) {
    // process_management
    atk_fork_bomb_register();               def_slot_quota_register();
    atk_kill_system_register();             def_kill_guard_register();
    atk_stack_overflow_register();          def_stack_guard_register();
    atk_zombie_leak_register();             def_reaper_register();
    atk_pid_wrap_register();                def_pid_bounds_register();
    atk_uninit_pcb_register();              def_pcb_wipe_register();
    atk_kernel_stack_to_user_register();    def_user_stack_register();
    atk_malicious_elf_register();           def_elf_validate_register();
    // process_management - new process attacks
    atk_signal_abuse_register();            def_signal_permission_register();
    atk_ptrace_abuse_register();            def_ptrace_restrict_register();
    atk_orphan_process_register();          def_orphan_reaping_register();
    atk_toctou_race_register();             def_atomic_check_register();
    atk_process_injection_register();       def_process_isolation_register();
    atk_process_hijacking_register();       def_control_flow_integrity_register();
    atk_container_escape_register();         def_namespace_isolation_register();
    atk_process_dos_register();             def_service_limits_register();
    atk_signal_flood_register();            def_sigqueue_limit_register();
    atk_sigreturn_forgery_register();       def_sigframe_check_register();
    atk_handler_hijack_register();          def_handler_bounds_register();
    atk_process_masquerade_register();      def_name_guard_register();
    atk_proc_info_leak_register();          def_hidepid_register();
    atk_kernel_pointer_leak_register();     def_kptr_hide_register();
    atk_pid_reuse_race_register();          def_pidfd_handle_register();
    atk_wait_steal_register();              def_child_only_register();
    atk_use_after_exit_register();          def_pcb_generation_register();
    atk_pgid_hijack_register();             def_session_check_register();
    atk_sleep_overflow_register();          def_sleep_bounds_register();
    atk_timer_flood_register();             def_timer_limits_register();
    atk_syscall_abuse_register();           def_seccomp_filter_register();
    atk_no_new_privs_bypass_register();     def_no_new_privs_register();
    atk_memory_peek_register();             def_vm_access_check_register();
    atk_argv_overflow_register();           def_arg_limits_register();
    atk_env_injection_register();           def_env_sanitize_register();
    atk_fd_leak_on_exec_register();         def_close_on_exec_register();
    atk_thread_exhaustion_register();       def_thread_quota_register();
    atk_stack_clash_register();             def_guard_gap_register();
    atk_core_dump_leak_register();          def_dumpable_flag_register();
    atk_priority_inversion_register();      def_priority_inheritance_register();
    atk_tty_hijack_register();              def_tty_guard_register();

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
    atk_umask_weak_register();              def_secure_umask_register();
    atk_huge_file_register();               def_size_cap_register();
    atk_timestamp_forgery_register();       def_time_owner_register();
    atk_dirent_leak_register();             def_dir_read_check_register();
    atk_long_path_register();               def_path_length_register();
    atk_deep_nesting_register();            def_depth_limit_register();
    atk_path_walk_bypass_register();        def_search_check_register();
    atk_symlink_race_register();            def_protected_symlinks_register();
    atk_symlink_loop_register();            def_link_depth_register();
    atk_hardlink_abuse_register();          def_protected_hardlinks_register();
    atk_nlink_overflow_register();          def_nlink_bounds_register();
    atk_use_after_unlink_register();        def_inode_refcount_register();

    // device_management
    atk_timer_divide_by_zero_register();    def_timer_frequency_register();
    atk_mouse_coordinate_escape_register(); def_mouse_bounds_register();
    atk_mouse_desync_register();            def_mouse_packet_sync_register();
    atk_irq_storm_register();               def_irq_mask_register();
    atk_canvas_overflow_register();         def_canvas_clip_register();
    atk_rtc_garbage_register();             def_rtc_validate_register();
    atk_invisible_text_register();          def_contrast_check_register();
    // device_management - new device attacks
    atk_keyboard_flood_register();          def_key_queue_bound_register();
    atk_speaker_abuse_register();           def_speaker_freq_guard_register();
    atk_bga_mode_overflow_register();       def_mode_bounds_register();
    atk_window_exhaustion_register();       def_window_quota_register();
    atk_window_size_overflow_register();    def_window_bounds_register();

    // io_management
    atk_invalid_descriptor_register();      def_descriptor_bounds_register();
    atk_oversized_transfer_register();      def_transfer_bounds_register();
    atk_fd_leak_register();                 def_fd_cleanup_register();

    // cpu_management
    atk_user_exception_register();          def_fault_isolation_register();
    atk_side_channel_register();              def_tsc_restrict_register();
    atk_priority_abuse_register();   def_priority_bounds_register();
    atk_starvation_register();                 def_aging_register();

    // cpu_management - new CPU attacks (resource / scheduling / privilege)
    atk_cpu_exhaustion_register();            def_cpu_time_limit_register();
    atk_infinite_loop_register();             def_cpu_watchdog_register();
    atk_resource_exhaustion_register();       def_resource_quota_register();
    atk_cpu_fork_bomb_register();             def_process_quota_register();
    atk_denial_of_service_register();         def_fair_scheduling_register();
    atk_scheduling_manipulation_register();   def_policy_restriction_register();
    atk_realtime_scheduling_abuse_register(); def_rt_throttling_register();
    atk_privilege_escalation_register();      def_permission_check_register();
    // cpu_management - microarchitecture (illustrations)
    atk_spectre_register();                   def_user_kernel_isolation_register();
    atk_meltdown_register();                  def_page_separation_register();
    atk_cache_timing_register();              def_coarse_timer_register();
    atk_branch_prediction_register();         def_speculation_barrier_register();
    // cpu_management - concepts (documented, not reproducible here)
    atk_distributed_denial_of_service_register(); def_rate_limiting_register();
    atk_rowhammer_register();                 def_ecc_and_refresh_register();
    atk_return_oriented_programming_register();    def_wx_and_aslr_register();
    atk_code_injection_register();            def_nx_pages_register();
    atk_hypervisor_escape_register();         def_vm_isolation_register();

    // user_management
    atk_setuid_escalation_register();       def_setuid_guard_register();
    atk_file_permission_bypass_register();  def_file_access_register();

    // shell
    atk_calc_overflow_register();           def_safe_arith_register();
    atk_line_overflow_register();           def_input_bound_register();

    // ui
    atk_output_spoof_register();            def_safe_text_register();

    // security_management
    atk_kernel_patch_register();            def_integrity_check_register();
    atk_log_flood_register();               def_audit_limit_register();
}
