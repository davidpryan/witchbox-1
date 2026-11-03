/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 61 (Witchbox, drums): Four Floor. A straight dance beat: a kick on every beat, an open hat
 * on every off-beat, a clap on 2 and 4, closed hats in the sixteenths, drop-outs at phrase ends. */
#include "toy_util.h"
#undef F
#define F(n) ff_##n

static const uint8_t FF_NEED[BAR] = {9, 2, 0, 5, 10, 3, 0, 7, 9, 1, 0, 6, 10, 4, 0, 8};
typedef struct { uint8_t sq[4]; int ring, bar_on, tick; } fourfloor_t;
#define DRIVE 0
#define HATS 1
#define CLAP 2
#define DROPOUTS 3

static void F(make)(toy_t *t)
{
    fourfloor_t *s = ST(fourfloor_t);
    add(t, "Drive", 0, 10, 4, "How crushed the kick is");
    add(t, "Hats", 0, 10, 5, "How busy the closed hats are");
    add(t, "Clap", 0, 10, 7, "How loud the clap is; 0 is off");
    add(t, "Drop-outs", 0, 10, 3, "How often the kick skips a phrase end");
    s->ring = s->bar_on = s->tick = -1;
}

static void F(randomise)(toy_t *t)
{
    P(DRIVE) = (int16_t)dice_int(11);
    P(HATS) = (int16_t)dice_int(11);
    P(CLAP) = (int16_t)dice_int(11);
    P(DROPOUTS) = (int16_t)dice_int(11);
}

static void F(settle)(fourfloor_t *s)
{
    int b;
    for (b = 0; b < 4; b++)
        s->sq[b] = 0;
    s->ring = s->bar_on = s->tick = -1;
}

static void F(play_changed)(toy_t *t) { F(settle)(ST(fourfloor_t)); }

static void F(on_step)(toy_t *t)
{
    fourfloor_t *s = ST(fourfloor_t);
    int i = t->steps % BAR, b = i / 4;
    if (i % 2 == 0)
        F(settle)(s);
    else
        s->tick = -1;
    if (i % 4 == 0) {
        int out = b == 3 && toy_bar(t) % 4 == 3 && dice_lt(P(DROPOUTS), 10);
        if (!out)
            toy_kick(t);
        s->sq[b] = out ? 2 : 1;
        if (b % 2 && P(CLAP)) {
            toy_note(t, 0, 1, 0);   /* the clap */
            s->ring = b;
        }
    } else if (i % 4 == 2) {
        toy_note(t, 0, 3, 0);       /* the open hat */
        s->bar_on = b;
    }
    if (FF_NEED[i] && P(HATS) >= FF_NEED[i]) {
        toy_note(t, 0, 2, 0);       /* a closed hat */
        s->tick = i;
    }
}

#ifdef WB_PICTURES
/* the picture: the four beats as the corners of a square in the seal, each an orb that fills in
 * the hue on the kick (less with Drive's crush) and goes sand when it drops out, a cream ring
 * round it for the clap; the open hats as points between the corners, lit as they sound, and the
 * closed hats' ticks on an inner ring, tall where they are played; the kick rune in the middle */
static void F(draw)(toy_t *t)
{
    fourfloor_t *s = ST(fourfloor_t);
    int cx = PIC_CX, cy = PIC_CY, b, i;
    pic_seal(cx, cy, 64, 4, 1, K_SAND);
    for (i = 0; i < BAR; i++) {
        int on = FF_NEED[i] && P(HATS) >= FF_NEED[i];
        if (i % 4 == 0)
            continue;
        if (i % 4 == 2)
            pic_pt(cx, cy, 58, i, BAR, 3, i / 4 == s->bar_on ? pic_hue : K_SOFT);
        else
            pic_pt(cx, cy, 34, i, BAR, i == s->tick ? 3 : on ? 2 : 0, i == s->tick ? K_TEXT : on ? K_SOFT : K_SAND);
    }
    for (b = 0; b < 4; b++) {
        int a = pic_ang(b * 4, BAR), x = pic_px(cx, 48, a), y = pic_py(cy, 48, a), sq = s->sq[b];
        pic_orb(x, y, 11, sq == 1 ? 1000 - P(DRIVE) * 40 : 0, sq == 2 ? K_SAND : K_TEXT, sq == 1 ? pic_hue : K_SAND);
        if (b == s->ring && P(CLAP)) {
            pic_circle(x, y, 15, K_TEXT);
            pic_circle(x, y, 16, K_TEXT);
        }
    }
    pic_rune(cx - 4, cy - 6, RUNE_KICK, K_TEXT);
}
#endif

TOY(toy_fourfloor, "fourfloor", "Four Floor", "four", ROLE_DRUMS, HUE_RED, 0, 1, "A straight dance beat: kick on every beat, open hats between, a clap on two and four.", fourfloor_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef DRIVE
#undef HATS
#undef CLAP
#undef DROPOUTS
