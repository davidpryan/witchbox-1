/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 109 (Witchbox IV, chords): Lichen. An L-system grows a rhythm: a string of long and short
 * notes rewritten a few times (long -> long short, short -> long), so the chords come in the
 * Fibonacci-like pattern of a lichen spreading. The organ holds each note for its length. */
#include "toy_util.h"
#undef F
#define F(n) lc_##n

#define LC_MAX 64
static const char *const LC_RULE_N[3] = {"Fibonacci", "Thue-Morse", "Triplets"};
typedef struct { uint8_t len[LC_MAX]; int n, at, left; } lichen_t;
#define GENERATIONS 0
#define RULE 1
#define HOLD 2
#define TOP 3

static void F(grow)(toy_t *t)
{
    lichen_t *s = ST(lichen_t);
    uint8_t a[LC_MAX], b[LC_MAX];
    int g, i, n = 1, m;
    a[0] = 2;                                   /* the axiom: one long note (lengths in steps / 2) */
    for (g = 0; g < P(GENERATIONS); g++) {
        m = 0;
        for (i = 0; i < n && m < LC_MAX - 3; i++) {
            switch (P(RULE)) {
            case 0:                             /* L -> L S, S -> L */
                if (a[i] == 2) { b[m++] = 2; b[m++] = 1; } else b[m++] = 2;
                break;
            case 1:                             /* L -> L S, S -> S L */
                if (a[i] == 2) { b[m++] = 2; b[m++] = 1; } else { b[m++] = 1; b[m++] = 2; }
                break;
            default:                            /* L -> S S S, S -> L */
                if (a[i] == 2) { b[m++] = 1; b[m++] = 1; b[m++] = 1; } else b[m++] = 2;
                break;
            }
        }
        for (i = 0; i < m; i++)
            a[i] = b[i];
        n = m;
    }
    for (i = 0; i < n; i++)
        s->len[i] = (uint8_t)(a[i] * (1 + P(HOLD)));
    s->n = n;
    s->at = 0;
    s->left = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Generations", 1, 5, 4, "How many times the pattern is rewritten");
    add_pick(t, "Rule", 3, 0, LC_RULE_N, "Which rewriting rule grows it");
    add(t, "Hold", 0, 3, 1, "How long a short note is: 1 to 4 steps");
    add(t, "Top", 0, 10, 4, "How often the top note of the chord moves");
    F(grow)(t);
}

static void F(randomise)(toy_t *t)
{
    P(GENERATIONS) = (int16_t)(2 + dice_int(4));
    P(RULE) = (int16_t)dice_int(3);
    P(HOLD) = (int16_t)dice_int(4);
    P(TOP) = (int16_t)dice_int(11);
    F(grow)(t);
}

static void F(changed)(toy_t *t) { F(grow)(t); }
static void F(play_changed)(toy_t *t) { ST(lichen_t)->at = ST(lichen_t)->left = 0; }

static void F(on_step)(toy_t *t)
{
    lichen_t *s = ST(lichen_t);
    int root, hold, top;
    if (s->left > 0) {
        s->left--;
        return;
    }
    if (s->at >= s->n)
        s->at = 0;
    s->left = s->len[s->at] - 1;
    hold = s->len[s->at] * t->step_ms - 20;
    root = toy_root(t, 0);
    top = dice_lt(P(TOP), 11) ? (dice_lt_q(32768) ? 6 : 7) : 4;
    toy_note(t, root, 0, hold);
    toy_note(t, root + 2, 0, hold);
    toy_note(t, root + top, 0, hold);
    s->at++;
}

TOY(toy_lichen, "lichen", "Lichen", "lichen", ROLE_CHORDS, HUE_GO, 0, 3,
    "An L-system grows the rhythm: long and short organ chords rewritten over and over, like lichen spreading.",
    lichen_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef GENERATIONS
#undef RULE
#undef HOLD
#undef TOP
