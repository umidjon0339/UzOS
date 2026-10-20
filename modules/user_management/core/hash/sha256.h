#ifndef SHA256_H
#define SHA256_H

/*
 * sha256.h - The SHA-256 hash function.
 *
 * A hash turns any amount of data into a fixed 32-byte "fingerprint":
 *
 *     sha256("guest")  -> 84983c60 f7daadc1 cb8698621f802c0d ...
 *
 * It only works one way: from the fingerprint there is no way back to
 * the data, short of guessing. That is why UzOS keeps the HASH of each
 * password, never the password itself (core/users).
 *
 * Use it in one go:              or piece by piece:
 *     sha256(data, len, out);        sha256_init(&ctx);
 *                                    sha256_update(&ctx, a, a_len);
 *                                    sha256_update(&ctx, b, b_len);
 *                                    sha256_final(&ctx, out);
 */

#include <stdint.h>
#include <stddef.h>

#define SHA256_SIZE 32              // bytes in a hash

typedef struct {
    uint32_t state[8];              // the running hash
    uint8_t  block[64];             // input waiting to fill a 64-byte block
    uint32_t block_len;
    uint64_t total_len;             // bytes hashed so far
} sha256_ctx_t;

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const void *data, size_t len);
void sha256_final(sha256_ctx_t *ctx, uint8_t out[SHA256_SIZE]);

// All three at once.
void sha256(const void *data, size_t len, uint8_t out[SHA256_SIZE]);

#endif
