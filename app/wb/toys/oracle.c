/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 112 (Witchbox IV, lead): Oracle. A melody from a chain of chances over the scale's steps: from
 * each note, the next is drawn from a table the seed dealt. Entropy flattens the table towards
 * anything-goes; at 0 the tune is fixed. */
#include "toy_util.h"
#undef F
#define F(n) or_##n

#define OR_N 8
typedef struct { uint8_t table[OR_N][OR_N]; int last; } oracle_t;
#define ENTROPY 0
#define RANGE 1
#define RESTS 2
#define RATE 3

static void F(deal)(toy_t *t)
{
    oracle_t *s = ST(oracle_t);
    int a, b;
    for (a = 0; a < OR_N; a++) {
        int fav = dice_int(OR_N);
        for (b = 0; b < OR_N; b++)
            s->table[a][b] = (uint8_t)(b == fav ? 20 : dice_int(4));
    }
    s->last = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Entropy", 0, 10, 3, "From one fixed tune to anything goes");
    add(t, "Range", 3, 8, 6, "How many scale steps it can use");
    add(t, "Rests", 0, 10, 3, "How often it rests");
    add(t, "Rate", 1, 4, 2, "Steps between notes");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(ENTROPY) = (int16_t)dice_int(11);
    P(RANGE) = (int16_t)(3 + dice_int(6));
    P(RESTS) = (int16_t)dice_int(11);
    P(RATE) = (int16_t)(1 + dice_int(4));
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    oracle_t *s = ST(oracle_t);
    int b, n = P(RANGE), w[OR_N], total = 0, r, next = 0;
    if (t->steps % P(RATE) != 0)
        return;
    if (dice_lt(P(RESTS), 16))
        return;
    for (b = 0; b < n; b++) {
        w[b] = s->table[s->last % n][b] + P(ENTROPY) * 2;
        total += w[b];
    }
    r = dice_int(total);
    for (b = 0; b < n; b++) {
        if (r < w[b]) {
            next = b;
            break;
        }
        r -= w[b];
    }
    s->last = next;
    toy_note(t, toy_root(t, 0) + 7 + next, 0, 0);
}

#ifdef WB_PICTURES
/* the picture: the table of chances as a grid, brighter where a step is likelier; the row of the note
 * just played lit, and the note it chose */
static void F(draw)(toy_t *t)
{
    oracle_t *s = ST(oracle_t);
    int a, b, n = P(RANGE), cell = 112 / n;
    for (a = 0; a < n; a++)
        for (b = 0; b < n; b++) {
            int w = s->table[a % n][b] + P(ENTROPY) * 2;
            uint16_t c = w >= 16 ? K_TEXT : w >= 8 ? K_SOFT : w >= 3 ? K_SAND : K_PANEL;
            if (a == s->last % n)
                c = b == s->last ? pic_hue : w >= 8 ? K_TEXT : K_SOFT;
            cv_rect(64 + b * cell, 12 + a * cell, cell - 1, cell - 1, c);
        }
    cv_text(4, 12, &FONT_XS, "FROM", K_DIM);
    cv_text(64, 128, &FONT_XS, "TO", K_DIM);
}
#endif

TOY(toy_oracle, "oracle", "Oracle", "oracle", ROLE_LEAD, HUE_PURPLE, 0, 3, "A melody from a chain of chances the seed dealt. Entropy turns one fixed tune into anything goes.", oracle_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef ENTROPY
#undef RANGE
#undef RESTS
#undef RATE
