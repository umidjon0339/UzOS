#ifndef SESSION_H
#define SESSION_H

/*
 * session.h - Who is logged in at the shell.
 *
 * The shell is a process (kernel_main). "Logging in as guest" simply
 * means: set that process's uid to guest's uid, and move it to guest's
 * home directory. From then on every check in the kernel (kill_guard,
 * priority_bounds, file permissions, ...) sees uid 1 and treats the
 * shell, and every process it starts, as guest.
 *
 *   boot -> session_login_prompt()   "uzos login:" until a name and
 *                                    password match (core/users)
 *        -> the shell runs as that user
 *   su   -> session_switch(other)    become someone else
 *   logout -> back to the login prompt
 */

#include "users.h"

#define SESSION_LINE_MAX 32

// Remember that the running process is the shell. Called once from
// kernel_main after the scheduler exists.
void session_init(void);

// Ask for a name and a password until they match. Blocks.
void session_login_prompt(void);

// Become 'user': the shell gets its uid and moves to its home directory.
void session_switch(const user_t *user);

// The user the shell runs as now.
const user_t *session_user(void);

// Read one typed line from the keyboard into 'out' (at most max - 1
// characters). hidden = 1: show nothing (for passwords).
// Returns 1 on Enter, 0 if Esc cancelled it.
int session_read_line(char *out, int max, int hidden);

// Ask "Password: " (hidden) and check it against 'user'.
#define PASSWORD_RIGHT      1
#define PASSWORD_WRONG      0
#define PASSWORD_CANCELLED -1       // Esc or Ctrl+C
int session_ask_password(const user_t *user);

// Ask for a new password twice (hidden). 1 = both the same and not empty.
// Typing it twice catches typos you cannot see. On 0, 'out' is wiped.
int session_ask_new_password(char *out, int max);

#endif
