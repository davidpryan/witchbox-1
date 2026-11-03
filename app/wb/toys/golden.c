/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 114 (Witchbox IV, lead): Golden. The Fibonacci word (long, short, long, long, short ...)
 * is the rhythm, and the Fibonacci numbers mod the scale are the notes: a music-box tune that
 * never repeats and never quite surprises. */
#include "toy_util.h"
#undef F
#define F(n) go_##n

typedef struct { int at, left, a, b, k; } golden_t;
#define SPAN 0
#define SHORT 1
#define MIRROR 2
#define OCTAVE 3

static void F(reset)(toy_t *t)
{
    golden_t *s = ST(golden_t);
    s->at = s->left = 0;
    s->a = 0;
    s->b = 1;
    s->k = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Span", 3, 12, 7, "How many scale steps the tune spans");
    add(t, "Short", 1, 3, 1, "How long a short note is (a long is golden: 1.6 times)");
    add(t, "Mirror", 0, 10, 0, "How often a note is mirrored downwards");
    add(t, "Octave", 0, 10, 3, "How often a note is lifted an octave");
    F(reset)(t);
}

static void F(randomise)(toy_t *t)
{
    P(SPAN) = (int16_t)(3 + dice_int(10));
    P(SHORT) = (int16_t)(1 + dice_int(3));
    P(MIRROR) = (int16_t)dice_int(11);
    P(OCTAVE) = (int16_t)dice_int(11);
    F(reset)(t);
}

static void F(play_changed)(toy_t *t) { F(reset)(t); }

/* the n-th letter of the Fibonacci word, 1 = long: a word built by S -> SL, L -> S */
static int F(word)(int n)
{
    /* floor((n + 2) * phi) - floor((n + 1) * phi) - 1, phi in 16.16 */
    const int32_t PHI = 106039;                 /* 1.6180339 * 65536 */
    return (int)((((int32_t)(n + 2) * PHI) >> 16) - (((int32_t)(n + 1) * PHI) >> 16) - 1);
}

static void F(on_step)(toy_t *t)
{
    golden_t *s = ST(golden_t);
    int deg, sh = P(SHORT), f;
    if (s->left > 0) {
        s->left--;
        return;
    }
    s->left = (F(word)(s->at) ? (sh * 13 + 4) / 8 : sh) - 1;   /* long = 1.6 x short, rounded up */
    if (s->left < 0)
        s->left = 0;
    s->at++;
    s->k++;
    f = s->b;                                   /* the next Fibonacci number, kept small */
    s->b = (s->a + s->b) % (P(SPAN) * 7);
    s->a = f;
    deg = f % P(SPAN);
    if (dice_lt(P(MIRROR), 11))
        deg = -deg;
    if (dice_lt(P(OCTAVE), 11))
        deg += 7;
    toy_note(t, toy_root(t, 0) + 7 + deg, 0, 0);
}

#ifdef WB_PICTURES
/* the picture: a golden spiral of squares, the Fibonacci squares, the one whose number plays now lit;
 * the rhythm's long and short as the frame under it */
static void F(draw)(toy_t *t)
{
    golden_t *s = ST(golden_t);
    static const uint8_t SQ[6][3] = {{152, 10, 88}, {64, 10, 88}, {64, 98, 34}, {98, 98, 34}, {98, 77, 21}, {119, 77, 21}};
    int i, k = s->k % 6;
    for (i = 0; i < 6; i++) {
        int x = SQ[i][0], y = SQ[i][1], w = SQ[i][2];
        cv_rect(x, y, w, w, i == k && t->playing ? pic_hue : K_PANEL);
        cv_rect(x, y, w, 1, K_SOFT); cv_rect(x, y, 1, w, K_SOFT);
        cv_rect(x, y + w - 1, w, 1, K_SOFT); cv_rect(x + w - 1, y, 1, w, K_SOFT);
    }
    cv_rect(8, 132, s->left > 0 ? 20 : 10, 3, K_TEXT);
}
#endif

TOY(toy_golden, "golden", "Golden", "spiral", ROLE_LEAD, HUE_YELLOW, 0, 3, "The Fibonacci word is the rhythm and the Fibonacci numbers the notes: a tune that never repeats and never quite surprises.", golden_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef SPAN
#undef SHORT
#undef MIRROR
#undef OCTAVE
