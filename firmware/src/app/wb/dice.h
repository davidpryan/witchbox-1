/* SPDX-License-Identifier: GPL-3.0-only */
/* The dice a toy rolls, from its seed: Playbox's dice.py, bit for bit.
 *
 * A 28-bit linear congruential generator worked in two 14-bit halves (so that on the Pico every
 * number stayed a small integer). Each roll gives a 16-bit fraction k, and Python's rand() was
 * k / 65536. The toys only ever used rand() in a few shapes, and each has an exact integer twin:
 *
 *     int(rand() * n)           dice_int(n)        (k * n) >> 16      (exact while k * n < 2^53)
 *     rand() < a / b            dice_lt(a, b)      k * b < 65536 * a  (a, b integers, b > 0)
 *     rand() < 0.37             dice_lt_q(DQ(0.37))k < ceil(0.37 * 65536)   (a constant)
 *     rand() * n < m            dice_mul_lt(n, m)  k * n < 65536 * m
 *     rand() * n >= m           !dice_mul_lt(n, m)
 *
 * Any other use is written out per toy with the same 64-bit integer sums. */
#pragma once
#include <stdint.h>

typedef struct { uint32_t s; uint16_t seed; } dice_t;

void dice_start(dice_t *d, int seed);
void dice_use(dice_t *d);
dice_t *dice_in_use(void);
uint32_t dice_k(void);                 /* one roll: the fraction's numerator, 0..65535 */
int dice_pick(int n);                  /* 0..n-1 (dice.pick, the same sum as int(rand() * n)) */
#define dice_int(n) dice_pick(n)
int dice_lt(int a, int b);             /* rand() < a / b */
int dice_lt_q(uint32_t q);             /* rand() < q / 65536 */
int dice_mul_lt(int n, int m);         /* rand() * n < m */
void dice_aside(dice_t *d, int seed, int salt);   /* a second generator for the same seed */
int dice_fresh(uint32_t entropy);      /* a new seed 0..9999 from 16 bits of entropy */

/* For rand() < p with a constant p, the threshold k must stay under is ceil(p * 65536) (k < p * 65536
 * exactly, with k an integer): 0.25 -> 16384, 0.5 -> 32768, 0.55 -> 36045, 0.6 -> 39322, 0.8 -> 52429.
 * The toys write these numbers in, worked out by hand, so no double appears even in a constant. */
