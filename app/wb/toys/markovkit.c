/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 103 (Witchbox IV, drums): Markov Kit. Each hit chooses the next from a table of chances dealt
 * by the seed: a chain of drums that has habits but no fixed bar. */
#include "toy_util.h"
#undef F
#define F(n) mk_##n

#define MK_N 6                                  /* kick snare hat open rim tom; 6 = a rest */
typedef struct { uint8_t table[MK_N + 1][MK_N + 1]; int last; } markovkit_t;
#define STICKY 0
#define RESTS 1
#define DOWNBEAT 2
#define COLOUR 3

static void F(deal)(toy_t *t)
{
    markovkit_t *s = ST(markovkit_t);
    int a, b;
    for (a = 0; a <= MK_N; a++)
        for (b = 0; b <= MK_N; b++)
            s->table[a][b] = (uint8_t)(1 + dice_int(8));
    s->last = MK_N;
}

static void F(make)(toy_t *t)
{
    add(t, "Sticky", 0, 10, 3, "How much a drum likes to repeat itself");
    add(t, "Rests", 0, 10, 4, "How often the chain rests");
    add(t, "Downbeat", 0, 10, 7, "How surely the kick lands on beat 1");
    add(t, "Colour", 0, 10, 4, "How often the rim and the tom get a turn");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(STICKY) = (int16_t)dice_int(11);
    P(RESTS) = (int16_t)dice_int(11);
    P(DOWNBEAT) = (int16_t)(4 + dice_int(7));
    P(COLOUR) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    markovkit_t *s = ST(markovkit_t);
    int i = t->steps % BAR, b, w[MK_N + 1], total = 0, r, next = MK_N;
    if (i == 0 && dice_lt(P(DOWNBEAT), 11)) {
        next = 0;
    } else {
        for (b = 0; b <= MK_N; b++) {
            w[b] = s->table[s->last][b];
            if (b == s->last)
                w[b] += P(STICKY) * 2;
            if (b == MK_N)
                w[b] += P(RESTS) * 3;
            if (b >= 4 && b < MK_N)
                w[b] = w[b] * P(COLOUR) / 10;
            total += w[b];
        }
        r = dice_int(total);
        for (b = 0; b <= MK_N; b++) {
            if (r < w[b]) {
                next = b;
                break;
            }
            r -= w[b];
        }
    }
    s->last = next;
    if (next == 0)
        toy_kick(t);
    else if (next < MK_N)
        toy_note(t, 0, next, 0);
}

TOY(toy_markovkit, "markovkit", "Markov Kit", "chain", ROLE_DRUMS, HUE_PURPLE, 0, 3,
    "Each hit chooses the next from a table of chances. A chain of drums with habits but no fixed bar.",
    markovkit_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef STICKY
#undef RESTS
#undef DOWNBEAT
#undef COLOUR
