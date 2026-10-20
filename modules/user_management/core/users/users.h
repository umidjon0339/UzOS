#ifndef USERS_H
#define USERS_H

/*
 * users.h - The user table: who may use this computer.
 *
 * Inside the kernel a user is just a NUMBER, the uid. Every process
 * carries one (process_t.uid), every file will carry one (its owner).
 * This table gives each number a name, a home directory and a password:
 *
 *   uid   name    home          password
 *   0     root    /home/root    salt + SHA-256 hash      (the administrator)
 *   1     guest   /home/guest   salt + SHA-256 hash      (a normal user)
 *   1000+ added with `useradd`
 *
 * uid 0 is special everywhere: root may do anything (kill any process,
 * read any file, set any priority). Every other uid is limited to what
 * it owns.
 *
 * Like Linux's /etc/passwd + /etc/shadow, but kept in kernel memory.
 */

#include <stdint.h>
#include "sha256.h"

#define USERS_MAX          16
#define USER_NAME_MAX      15       // "a"-"z", "0"-"9", "_"; starts with a letter
#define USER_HOME_MAX      32
#define USER_PASSWORD_MAX  32
#define USER_SALT_SIZE     16

#define UID_ROOT           0
#define UID_GUEST          1
#define UID_FIRST_ADDED    1000     // useradd starts here

typedef struct {
    int      used;                  // 0 = free slot
    uint32_t uid;
    char     name[USER_NAME_MAX + 1];
    char     home[USER_HOME_MAX];   // "/home/guest"
    uint8_t  salt[USER_SALT_SIZE];  // random bytes, different for every user
    uint8_t  hash[SHA256_SIZE];     // SHA-256 of salt + password, repeated
} user_t;

// Create root and guest and their home directories. Called from
// kernel_main once the filesystem exists.
void users_init(void);

// Look a user up. NULL if there is none.
const user_t *users_find_name(const char *name);
const user_t *users_find_uid(uint32_t uid);

// The user's name, or "#1005" for a uid nobody has any more.
void users_name(uint32_t uid, char *out, int size);

// Is 'password' the user's password? 1 = yes.
int users_check_password(const user_t *user, const char *password);

// Give 'uid' a new password. 1 = done, 0 = no such user / too long.
int users_set_password(uint32_t uid, const char *password);

// A good user name? 1-15 of "a"-"z", "0"-"9", "_", starting with a letter.
int users_name_ok(const char *name);

// Add a user with a home directory. Returns the new uid, or -1 (bad or
// taken name, table full). uids count up from UID_FIRST_ADDED and are
// never handed out again, even after the user is deleted.
int users_add(const char *name, const char *password);

// Remove a user from the table. Its files keep its uid (shown as
// "#1005"). 1 = done, 0 = no such user, or root.
int users_delete(uint32_t uid);

// Walk the table: index 0 .. USERS_MAX - 1, NULL for a free slot.
const user_t *users_slot(int index);
int users_count(void);

#endif
