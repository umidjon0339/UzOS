/*
 * cmd_testuser.c - `testuser [null|badptr]`: run a tiny program in ring 3.
 *
 *   testuser         calls write, getpid, uptime, mmap, munmap, exit via int 0x80
 *   testuser null    writes to address 0: the page fault kills only it
 *   testuser badptr  passes write() an unmapped buffer: only that call fails
 *
 * The program is copied to the user pages and dropped to ring 3 by
 * usertask (core/usertask). From there it can only reach the kernel
 * through int 0x80; each call is traced on the screen by the syscall
 * handler.
 *
 * Uses: usertask_start (core/usertask), sys_sleep, ui_* (ui module).
 */

#include "proc_commands.h"
#include "usertask.h"
#include "string.h"
#include "sys_sleep.h"
#include "ui.h"

#define USER_UID 1

// From user_program.s
extern uint8_t user_program[], user_program_end[];
extern uint8_t user_null_program[], user_null_program_end[];
extern uint8_t user_badptr_program[], user_badptr_program_end[];

void cmd_testuser(const char *args) {
    const uint8_t *start = user_program, *end = user_program_end;

    if (strcmp(args, "null") == 0) {
        start = user_null_program;
        end   = user_null_program_end;
    } else if (strcmp(args, "badptr") == 0) {
        start = user_badptr_program;
        end   = user_badptr_program_end;
    }

    int pid = usertask_start("usertest", start, end, USER_UID);

    if (pid < 0) {
        ui_error("testuser", "could not create the process");
        return;
    }
    ui_start(UI_OK, "testuser");
    ui_printf("started the ring 3 test, PID %d", pid);
    ui_end();

    sys_sleep(200);     // let it run, so its trace appears before the prompt
}
