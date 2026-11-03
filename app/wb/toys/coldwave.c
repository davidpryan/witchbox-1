/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 87 (Witchbox III, bass): Cold Wave. A hard, metallic bass repeating one rigid sixteen-step
 * pattern on the root, with octave jumps on some steps and, sometimes, a drop at the end of the bar. */
#include "toy_util.h"
#undef F
#define F(n) cw_##n

static const uint8_t CW_HOLD[2] = {1, 2};
static const char *const CW_HOLD_N[2] = {"Short", "Long"};
typedef struct { uint8_t rank[BAR], jump[BAR], on[BAR]; } coldwave_t;
#define DENSITY 0
#define JUMPS 1
#define DROPS 2
#define HOLD 3

static void F(lay)(toy_t *t)
{
    coldwave_t *s = ST(coldwave_t);
    int i;
    for (i = 0; i < BAR; i++)
        s->on[i] = i == 0 || s->rank[i] < P(DENSITY);
}

static void F(deal)(toy_t *t)
{
    coldwave_t *s = ST(coldwave_t);
    int i;
    for (i = 0; i < BAR; i++) {
        s->rank[i] = (uint8_t)dice_int(10);
        s->jump[i] = (uint8_t)dice_int(10);
    }
    F(lay)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Density", 0, 10, 6, "How many of the 16 dots the bass plays");
    add(t, "Jumps", 0, 10, 4, "How many steps jump up an octave");
    add(t, "Drops", 0, 10, 3, "How often the bar ends an octave down");
    add_pick(t, "Hold", 2, 0, CW_HOLD_N, "How long each note lasts");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(DENSITY) = (int16_t)(3 + dice_int(8));
    P(JUMPS) = (int16_t)dice_int(11);
    P(DROPS) = (int16_t)dice_int(11);
    P(HOLD) = (int16_t)dice_int(2);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(lay)(t); }

static void F(on_step)(toy_t *t)
{
    coldwave_t *s = ST(coldwave_t);
    int i = t->steps % BAR, deg;
    if (!s->on[i])
        return;
    deg = toy_root(t, 0) - 7;
    if (i % 2 == 1 && s->jump[i] < P(JUMPS))
        deg += 7;
    if (i >= 14 && dice_lt(P(DROPS), 11))
        deg -= 7;
    toy_note(t, deg, 0, t->step_ms * CW_HOLD[P(HOLD)] - 10);
}

#ifdef WB_PICTURES
/* the picture: the rigid pattern as two rows of sixteen cells, the root's row and the octave's, a cell
 * filled where the bass hits; the step sounding in colour */
static void F(draw)(toy_t *t)
{
    coldwave_t *s = ST(coldwave_t);
    int i, now = t->steps % BAR;
    for (i = 0; i < BAR; i++) {
        int x = 8 + i * 14, up = i % 2 == 1 && s->jump[i] < P(JUMPS);
        cv_rect(x, 44, 11, 28, K_PANEL);
        cv_rect(x, 80, 11, 28, K_PANEL);
        if (s->on[i])
            cv_rect(x, up ? 44 : 80, 11, 28, i == now && t->playing ? pic_hue : K_TEXT);
    }
    pic_bar(t->steps, BAR);
}
#endif

TOY(toy_coldwave, "coldwave", "Cold Wave", "wave", ROLE_BASS, HUE_CYAN, 0, 2, "A hard, metallic bass repeating one rigid pattern, with octave jumps.", coldwave_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef DENSITY
#undef JUMPS
#undef DROPS
#undef HOLD
