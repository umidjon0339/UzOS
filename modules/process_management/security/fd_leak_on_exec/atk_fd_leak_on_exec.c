/*
 * atk_fd_leak_on_exec.c - ATTACK fd_leak_on_exec: inherit a file you may
 * not open.
 *
 * Breaks:     disclosure - an open file belongs to the program that opened it
 * Stopped by: close_on_exec (defence/close_on_exec)
 *
 * execve keeps the descriptor table. A privileged program that opened a
 * secret and then runs a helper hands the helper that open descriptor:
 * the helper reads the secret without ever passing a permission check.
 * O_CLOEXEC marks a descriptor "mine only"; execve must close it.
 *
 * simulate(): explain.
 * launch():   a test process holds fd 3 = "secret" (O_CLOEXEC) and
 *             fd 4 = "log" (no flag); close_on_exec runs as at execve.
 *             findings = 1 if the secret is still open after it.
 *             (`proccalls exec` checks it with a real execve.)
 * cleanup():  nothing (the test process is not a real one).
 *
 * Uses: fd_alloc (io_management core/fd), O_CLOEXEC / O_READ
 *       (io_management), def_close_on_exec (its defence), memset, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/fd_leak_on_exec/defence/close_on_exec/def_close_on_exec.h"  // def_close_on_exec
#include "modules/io_management/core/fd/fd.h"                     // fd_alloc
#include "modules/io_management/syscalls/open/sys_open.h"         // O_CLOEXEC, O_READ
#include "lib/string/string.h"                                    // memset, strncpy
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

static process_t victim;            // a stand-in: only its descriptor table is used
static vfs_node_t secret, log_file;

static int simulate(void) {
    ui_printf("  root opens /etc/secret, then execve(\"helper\")\n");
    ui_printf("  without O_CLOEXEC the helper inherits fd 3: it reads the secret\n");
    return 1;
}

static int launch(void) {
    memset(&victim, 0, sizeof(victim));
    memset(&secret, 0, sizeof(secret));
    memset(&log_file, 0, sizeof(log_file));
    strncpy(secret.name, "secret", FS_NAME_MAX);
    strncpy(log_file.name, "log", FS_NAME_MAX);
    int fd_secret = fd_alloc(&victim, &secret, O_READ | O_CLOEXEC);
    int fd_log    = fd_alloc(&victim, &log_file, O_READ);

    int closed = def_close_on_exec(&victim);
    int leaked = victim.fd_table[fd_secret] != 0;
    int kept   = victim.fd_table[fd_log] != 0;

    ui_printf("  execve closed %d descriptor(s)\n", closed);
    ui_printf("  fd %d \"secret\" (O_CLOEXEC)   ", fd_secret);
    ui_color(leaked ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", leaked ? "still open in the helper!" : "closed");
    ui_reset();
    ui_printf("  fd %d \"log\" (no flag)        ", fd_log);
    ui_color(kept ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", kept ? "still open (as asked)" : "closed (too eager!)");
    ui_reset();
    if (kept)                                       // tidy the stand-in up
        victim.fd_table[fd_log] = 0;
    log_file.open_count = 0;
    return leaked;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "fd_leak_on_exec",
    "a helper exec'd by a privileged program inherits its secret file",
    INV_DISCLOSURE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "close_on_exec"
};

void atk_fd_leak_on_exec_register(void) { registry_add_attack(&attack); }
