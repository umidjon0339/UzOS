/*
 * users.c - Keep the user table and check passwords.
 *
 * How a password is stored
 * ------------------------
 *   salt  = 16 fresh bytes, made when the password is set
 *   hash  = SHA-256(salt + password), then SHA-256(hash + salt) again and
 *           again, HASH_ROUNDS times in all
 *
 * The password itself is never kept. To check a login we compute the
 * same thing from the typed password and compare the two hashes.
 *
 * Why a salt: two users with the same password still get different
 * hashes, so the table does not show who shares a password.
 * Why many rounds: checking one password takes a little longer, which
 * the user never notices, but trying millions of them takes far longer.
 *
 * Each user's home belongs to that user. root's is private (mode 60),
 * the others may be looked at by everyone (mode 64), like on Linux.
 *
 * Uses: sha256 (core/hash), tsc_now (cpu_management) and timer_ticks
 *       (device_management) to make salts, ramdisk_create + vfs_lookup
 *       (filesystem_management) for /home, MODE_* (core/perm),
 *       format / string (lib).
 */

#include "users.h"
#include "string.h"
#include "format.h"
#include "tsc.h"
#include "timer.h"
#include "ramdisk.h"
#include "perm.h"

#define HASH_ROUNDS 1000

static user_t table[USERS_MAX];

// The uid the next `useradd` gets. It only ever goes up: if uid 1000 were
// given again after its user was deleted, the new user would silently
// own every file the old one left behind.
static uint32_t next_uid = UID_FIRST_ADDED;

/* ---------- Passwords ---------- */

// 16 bytes that are different every time. The cycle counter and the tick
// count change constantly; hashing them spreads every changed bit over
// all the bytes. (A real OS would also use a hardware random generator.)
static void make_salt(uint8_t salt[USER_SALT_SIZE]) {
    static uint32_t counter;
    struct { uint64_t cycles; uint32_t ticks, counter; } seed = {
        tsc_now(), timer_ticks, ++counter
    };
    uint8_t mixed[SHA256_SIZE];

    sha256(&seed, sizeof(seed), mixed);
    memcpy(salt, mixed, USER_SALT_SIZE);
}

static void hash_password(const uint8_t salt[USER_SALT_SIZE], const char *password,
                          uint8_t out[SHA256_SIZE]) {
    sha256_ctx_t ctx;

    sha256_init(&ctx);                              // round 1: salt + password
    sha256_update(&ctx, salt, USER_SALT_SIZE);
    sha256_update(&ctx, password, strlen(password));
    sha256_final(&ctx, out);

    for (int round = 1; round < HASH_ROUNDS; round++) {
        sha256_init(&ctx);                          // rounds 2..: hash + salt
        sha256_update(&ctx, out, SHA256_SIZE);
        sha256_update(&ctx, salt, USER_SALT_SIZE);
        sha256_final(&ctx, out);
    }
}

int users_check_password(const user_t *user, const char *password) {
    if (!user || !password)
        return 0;

    uint8_t typed[SHA256_SIZE];
    hash_password(user->salt, password, typed);

    // Look at every byte, even after a difference: the answer always
    // takes the same time, whatever was typed.
    uint8_t difference = 0;
    for (int i = 0; i < SHA256_SIZE; i++)
        difference |= typed[i] ^ user->hash[i];
    return difference == 0;
}

static user_t *find_uid(uint32_t uid) {
    for (int i = 0; i < USERS_MAX; i++)
        if (table[i].used && table[i].uid == uid)
            return &table[i];
    return 0;
}

int users_set_password(uint32_t uid, const char *password) {
    user_t *user = find_uid(uid);
    if (!user || !password || strlen(password) > USER_PASSWORD_MAX)
        return 0;

    make_salt(user->salt);                          // a new password gets a new salt
    hash_password(user->salt, password, user->hash);
    return 1;
}

/* ---------- Home directories ---------- */

// "/home/<name>", created if missing, owned by 'uid'.
static void make_home(const char *name, uint32_t uid) {
    vfs_node_t *home = vfs_lookup(fs_root, "/home");
    if (!home)
        home = ramdisk_create(fs_root, "home", FS_DIRECTORY);
    if (!home)
        return;

    vfs_node_t *dir = vfs_finddir(home, name);
    if (!dir)
        dir = ramdisk_create(home, name, FS_DIRECTORY);
    if (dir) {
        dir->owner = uid;
        dir->mode  = uid == UID_ROOT ? MODE_PRIVATE : MODE_DEFAULT;
    }
}

/* ---------- The table ---------- */

// Put a user in a free slot. Returns it, or NULL if the table is full.
static user_t *add(uint32_t uid, const char *name, const char *password) {
    for (int i = 0; i < USERS_MAX; i++) {
        user_t *user = &table[i];
        if (user->used)
            continue;

        memset(user, 0, sizeof(*user));
        user->used = 1;
        user->uid  = uid;
        strncpy(user->name, name, USER_NAME_MAX);
        format(user->home, sizeof(user->home), "/home/%s", name);
        make_home(name, uid);
        users_set_password(uid, password);
        return user;
    }
    return 0;
}

void users_init(void) {
    memset(table, 0, sizeof(table));
    add(UID_ROOT,  "root",  "uzos");
    add(UID_GUEST, "guest", "guest");
}

int users_name_ok(const char *name) {
    int len = strlen(name);
    if (len < 1 || len > USER_NAME_MAX || name[0] < 'a' || name[0] > 'z')
        return 0;
    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'))
            return 0;
    }
    return 1;
}

int users_add(const char *name, const char *password) {
    if (!users_name_ok(name) || users_find_name(name))
        return -1;
    user_t *user = add(next_uid, name, password);
    if (!user)
        return -1;                                  // table full
    return (int)next_uid++;
}

int users_delete(uint32_t uid) {
    user_t *user = find_uid(uid);
    if (!user || uid == UID_ROOT)
        return 0;                                   // root can never go
    memset(user, 0, sizeof(*user));                 // the hash goes too
    return 1;
}

const user_t *users_find_name(const char *name) {
    for (int i = 0; i < USERS_MAX; i++)
        if (table[i].used && strcmp(table[i].name, name) == 0)
            return &table[i];
    return 0;
}

const user_t *users_find_uid(uint32_t uid) {
    return find_uid(uid);
}

void users_name(uint32_t uid, char *out, int size) {
    const user_t *user = find_uid(uid);
    if (user)
        format(out, size, "%s", user->name);
    else
        format(out, size, "#%u", uid);              // nobody has this uid any more
}

const user_t *users_slot(int index) {
    if (index < 0 || index >= USERS_MAX || !table[index].used)
        return 0;
    return &table[index];
}

int users_count(void) {
    int n = 0;
    for (int i = 0; i < USERS_MAX; i++)
        n += table[i].used;
    return n;
}
