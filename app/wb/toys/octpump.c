/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 48 (Witchbox, bass): Octave Pump. A buzzing bass hammering the chord's root; notes between
 * the beats may jump up an octave, rolled afresh every bar. */
#include "toy_util.h"
#undef F
#define F(n) op_##n

static const char *const OP_PAT_N[3] = {"Off-beat", "Straight", "Gallop"};
static const uint8_t OP_WHEN[3] = {0x5, 0xF, 0xD};      /* steps of each beat that sound (bits) */
static const uint8_t OP_UP[3] = {0x4, 0xA, 0xC};        /* and those that may jump */

typedef struct { uint8_t cells[BAR]; int lit; } octpump_t;   /* 0 silent, 1 low, 2 high */

#define PATTERN 0
#define OCTAVES 1
#define GATE 2
#define BITE 3

static void F(roll)(toy_t *t)
{
    octpump_t *s = ST(octpump_t);
    int when = OP_WHEN[P(PATTERN)], up = OP_UP[P(PATTERN)], i;
    for (i = 0; i < BAR; i++) {
        int b = 1 << (i % 4);
        if (!(when & b))
            s->cells[i] = 0;
        else if ((up & b) && dice_lt(P(OCTAVES), 10))
            s->cells[i] = 2;
        else
            s->cells[i] = 1;
    }
}

static void F(make)(toy_t *t)
{
    add_pick(t, "Pattern", 3, 0, OP_PAT_N, "The rhythm it hammers out");
    add(t, "Octaves", 0, 10, 7, "How often a note jumps up an octave");
    add(t, "Gate", 1, 10, 5, "From short stabs to joined notes");
    add(t, "Bite", 0, 10, 5, "How bright the tone is");
    ST(octpump_t)->lit = -1;
    F(roll)(t);
}

static void F(randomise)(toy_t *t)
{
    P(PATTERN) = (int16_t)dice_int(3);
    P(OCTAVES) = (int16_t)dice_int(11);
    P(GATE) = (int16_t)(1 + dice_int(10));
    P(BITE) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t) { F(roll)(t); }

static void F(play_changed)(toy_t *t)
{
    if (!t->playing)
        ST(octpump_t)->lit = -1;
}

static void F(on_step)(toy_t *t)
{
    octpump_t *s = ST(octpump_t);
    int i = t->steps % BAR, cell;
    if (i == 0)
        F(roll)(t);
    cell = s->cells[i];
    s->lit = cell ? i : -1;
    if (cell)
        toy_note(t, cell == 1 ? toy_root(t, 0) - 7 : toy_root(t, 0), 0, 0);
}

#ifdef WB_PICTURES
/* the picture: the bar as a wheel with two rings, low and high; a point on the inner ring for a
 * step that hammers the root, on the outer for one that jumps the octave, a sand dot for a rest;
 * fatter with Gate; the one sounding now in the hue, the needle on the step */
static void F(draw)(toy_t *t)
{
    octpump_t *s = ST(octpump_t);
    int cx = PIC_CX, cy = PIC_CY, size = 2 + P(GATE) * 3 / 10, i;
    pic_circle(cx, cy, 64, K_SAND);
    pic_circle(cx, cy, 58, K_SAND);
    pic_circle(cx, cy, 42, K_SAND);
    pic_needle(cx, cy, 36, t->steps % BAR, BAR, K_LINE);
    for (i = 0; i < BAR; i++) {
        int row = s->cells[i];
        if (!row)
            pic_pt(cx, cy, 42, i, BAR, 0, K_SAND);
        else
            pic_pt(cx, cy, row == 1 ? 42 : 58, i, BAR, i == s->lit ? size + 1 : size, i == s->lit ? pic_hue : K_TEXT);
    }
    pic_rune(cx - 4, cy - 6, RUNE_BASS, s->lit >= 0 && s->cells[s->lit] == 2 ? pic_hue : K_TEXT);
}
#endif

TOY(toy_octpump, "octpump", "Octave Pump", "oct", ROLE_BASS, HUE_GO, 0, 0, "A buzzing bass that hammers the root and jumps up an octave on the off-beats.", octpump_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef PATTERN
#undef OCTAVES
#undef GATE
#undef BITE
