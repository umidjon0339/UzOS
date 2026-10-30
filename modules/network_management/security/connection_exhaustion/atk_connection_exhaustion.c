/*
 * atk_connection_exhaustion.c - ATTACK connection_exhaustion: take every TCP connection slot
 *
 * Breaks:     resource - one user cannot take every TCP connection slot
 * Stopped by: conn_quota (defence/conn_quota)
 *
 * The kernel has room for TCP_MAX_CONNS connections. A user who opens
 * them all (and keeps them) leaves none for anyone else: no ssh, no web.
 *
 * launch(): as uid 1, create 16 connections. findings = those beyond
 *           the user's quota.
 * cleanup(): free them.
 *
 * Uses: tcp_new / tcp_free (core/tcp), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/connection_exhaustion/defence/conn_quota/def_conn_quota.h" // TCP_USER_MAX
#include "modules/network_management/core/tcp/tcp.h"                            // tcp_new, tcp_free, TCP_MAX_CONNS
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define ATTACKER_UID 1
#define TRIES 16
static tcp_conn_t *taken[TRIES];

static int simulate(void) {
    ui_printf("  %d TCP connection slots for everyone together\n", TCP_MAX_CONNS);
    ui_printf("  one user could hold them all; the quota is %d per user\n", TCP_USER_MAX);
    return 1;
}

static int launch(void) {
    int got = 0;
    for (int i = 0; i < TRIES; i++) {
        taken[i] = tcp_new(ATTACKER_UID);
        got += taken[i] != 0;
    }
    int extra = got > TCP_USER_MAX ? got - TCP_USER_MAX : 0;
    ui_printf("  uid %d asked for %d connections, got %d\n", ATTACKER_UID, TRIES, got);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "it holds more than its share" : "refused after its quota: slots stay free for others");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    int freed = 0;
    for (int i = 0; i < TRIES; i++) {
        if (taken[i]) {
            tcp_free(taken[i]);
            taken[i] = 0;
            freed++;
        }
    }
    return freed;
}

static const attack_t attack = {
    "connection_exhaustion",
    "as a normal user, open TCP connections until none are left",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "conn_quota"
};

void atk_connection_exhaustion_register(void) { registry_add_attack(&attack); }
