/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 84 (Witchbox III, drums): Rattlebones. Dry wooden notes in short figures of an odd length,
 * so they turn against the beat, each note struck once or a few times, then a silence. */
#include "toy_util.h"
#undef F
#define F(n) rb_##n

static const uint8_t RB_LEN[3] = {3, 5, 7};
static const char *const RB_FIG_N[3] = {"3 notes", "5 notes", "7 notes"};
typedef struct { int8_t fig[7]; int at, left, rest; } rattle_t;
#define FIGURE 0
#define STROKES 1
#define REST 2
#define SPREAD 3

static void F(deal)(toy_t *t)
{
    rattle_t *s = ST(rattle_t);
    int i;
    for (i = 0; i < 7; i++)
        s->fig[i] = (int8_t)(dice_int(2 * P(SPREAD) + 1) - P(SPREAD));
}

static void F(make)(toy_t *t)
{
    add_pick(t, "Figure", 3, 1, RB_FIG_N, "How many notes the figure has");
    add(t, "Strokes", 1, 4, 1, "How many times it is struck each time");
    add(t, "Rest", 0, 10, 3, "How long the silence after the figure is");
    add(t, "Spread", 0, 7, 3, "How far the notes stray from the root");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(FIGURE) = (int16_t)dice_int(3);
    P(STROKES) = (int16_t)(1 + dice_int(4));
    P(REST) = (int16_t)dice_int(11);
    P(SPREAD) = (int16_t)dice_int(8);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(deal)(t); }

static void F(play_changed)(toy_t *t)
{
    rattle_t *s = ST(rattle_t);
    s->at = s->left = s->rest = 0;
}

static void F(on_step)(toy_t *t)
{
    rattle_t *s = ST(rattle_t);
    int n = RB_LEN[P(FIGURE)];
    if (s->rest > 0) {
        s->rest--;
        return;
    }
    if (s->left == 0) {                         /* the next note of the figure */
        s->left = P(STROKES);
    }
    toy_note(t, toy_root(t, 0) + s->fig[s->at], 0, 0);
    if (--s->left == 0) {
        if (++s->at >= n) {
            s->at = 0;
            s->rest = P(REST) * 2 + dice_int(3);
        }
    }
}

TOY(toy_rattle, "rattle", "Rattlebones", "bones", ROLE_DRUMS, HUE_YELLOW, 0, 2,
    "Dry wooden notes in short figures that turn against the beat.",
    rattle_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef FIGURE
#undef STROKES
#undef REST
#undef SPREAD
