#ifndef WATCHDOG_H
#define WATCHDOG_H

/*
 * watchdog.h - Notices a live attack that runs too long.
 *
 * `seclab attack <id> live` arms it with a time budget. If the attack is
 * still running when the budget runs out, the timer interrupt marks it
 * as overrun and writes it to the audit log. It reports; it does not
 * stop the attack.
 */

#include <stdint.h>

void watchdog_arm(uint32_t ticks, const char *attack_id);
void watchdog_disarm(void);
void watchdog_tick(void);                   // called by the timer interrupt
int  watchdog_fired(void);                  // 1 = the last run overran
const char *watchdog_attack(void);          // which attack it was

#endif
