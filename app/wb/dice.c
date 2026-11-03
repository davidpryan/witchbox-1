/* SPDX-License-Identifier: GPL-3.0-only */
/* Playbox's dice.py in C (see dice.h). */
#include "dice.h"

#define A 12829u                        /* multiplier; with an odd step the period is 2^28 */
#define M 0xFFFFFFFu

static dice_t boot_box = {0x1234567u, 0};
static dice_t *box = &boot_box;

static inline uint32_t step(uint32_t s)
{
    return (((((s >> 14) * A) & 0x3FFFu) << 14) + (s & 0x3FFFu) * A + 12345u) & M;
}

void dice_start(dice_t *d, int seed)
{
    uint32_t s = ((uint32_t)seed * 9973u + 4057u) & M;
    int i;
    for (i = 0; i < 3; i++)             /* stir, so neighbouring seeds part company */
        s = step(s);
    d->s = s;
    d->seed = (uint16_t)seed;
}

void dice_use(dice_t *d) { box = d; }
dice_t *dice_in_use(void) { return box; }

uint32_t dice_k(void)
{
    box->s = step(box->s);
    return box->s >> 12;
}

/* 32-bit sums only (the device has no 64-bit multiply to lean on): k < 65536, so these are exact
 * while n < 65536, a and b < 32768, and n * 65536 and m * 65536 stay under 2^31 — true of every
 * use in the toys (the largest: a seed, 10000; pops * ms, 1500). */
int dice_pick(int n) { return (int)((dice_k() * (uint32_t)n) >> 16); }
int dice_lt(int a, int b) { return (int32_t)(dice_k() * (uint32_t)b) < a * 65536; }
int dice_lt_q(uint32_t q) { return dice_k() < q; }
int dice_mul_lt(int n, int m) { return (int32_t)(dice_k() * (uint32_t)n) < m * 65536; }

void dice_aside(dice_t *d, int seed, int salt) { dice_start(d, (seed + 10007 * salt) & 0xFFFF); }
int dice_fresh(uint32_t entropy) { return (int)(((entropy & 0xFFFFu) * 10000u) >> 16); }
