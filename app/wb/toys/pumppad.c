/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 51 (Witchbox, chords): Pump Pad. A held chord that ducks on every kick (a drummer's in the
 * band, its own beat alone) and swells back. level 0..100 is for the sound engine. */
#include "toy_util.h"
#undef F
#define F(n) pp_##n

static const char *const PP_COLOUR_N[3] = {"Triad", "Add 7", "Add 9"};
typedef struct { int level, lvl; } pumppad_t;
#define DEPTH 0
#define RECOVERY 1
#define COLOUR 2
#define SPREAD 3

static void F(make)(toy_t *t)
{
    pumppad_t *s = ST(pumppad_t);
    add(t, "Depth", 0, 10, 8, "How far the volume ducks on a kick");
    add(t, "Recovery", 1, 10, 5, "How fast it swells back");
    add_pick(t, "Colour", 3, 1, PP_COLOUR_N, "How many notes are in the chord");
    add(t, "Spread", 0, 10, 4, "From close notes to a wide chord");
    s->level = 100;
    s->lvl = 1000;
}

static void F(randomise)(toy_t *t)
{
    P(DEPTH) = (int16_t)(3 + dice_int(8));
    P(RECOVERY) = (int16_t)(1 + dice_int(10));
    P(COLOUR) = (int16_t)dice_int(3);
    P(SPREAD) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t) { ST(pumppad_t)->lvl = 1000; }

static void F(duck)(toy_t *t)
{
    pumppad_t *s = ST(pumppad_t);
    s->lvl = imin(s->lvl, 1000 - P(DEPTH) * 100);
}

static void F(on_step)(toy_t *t)
{
    int i = t->steps % BAR;
    if (i == 0) {                   /* a new bar: sound the chord */
        int r = toy_root(t, 0), sp = P(SPREAD), k;
        for (k = 0; k < 3 + P(COLOUR); k++) {
            int deg = r + 2 * k;
            if ((k == 1 && sp >= 3) || (k == 3 && sp >= 9))
                deg += 7;           /* wider: lift the third, then the seventh */
            if (k == 0 && sp >= 6)
                deg -= 7;           /* and drop the root */
            toy_note(t, deg, k, 0);
        }
    }
    if (!S.drummer && i % 4 == 0)   /* nobody is drumming: pump on the beat */
        F(duck)(t);
}

static void F(frame)(toy_t *t, int ms)
{
    pumppad_t *s = ST(pumppad_t);
    if (!t->playing)
        return;
    if (S.drummer && toy_new_kick(t))
        F(duck)(t);
    if (s->lvl < 1000)              /* back to full in 1.2 s down to 0.15 s */
        s->lvl = imin(1000, s->lvl + ms * 1000 / (1320 - P(RECOVERY) * 117));
    s->level = s->lvl / 10;
}

#ifdef WB_PICTURES
/* the picture: a blob that squashes flat on every kick and swells back, round-cornered, in colour while
 * it is ducked */
static void F(draw)(toy_t *t)
{
    pumppad_t *s = ST(pumppad_t);
    int w = 72 + P(SPREAD) * 15, h = (18 + 75 * s->lvl / 1000) & ~3, r = 9, cx = 120, cy = 72, y;
    uint16_t c = s->lvl < 1000 ? pic_hue : K_TEXT;
    if (h < 2 * r)
        r = h / 2;
    cv_rect(cx - w / 2, cy - h / 2, w, h, c);
    /* the corners, cut back to round */
    for (y = 0; y < r; y++) {
        int dy = r - y, dx = r;
        while (dx > 0 && dx * dx + dy * dy > r * r)
            dx--;
        cv_rect(cx - w / 2, cy - h / 2 + y, r - dx, 1, K_BG);
        cv_rect(cx + w / 2 - (r - dx), cy - h / 2 + y, r - dx, 1, K_BG);
        cv_rect(cx - w / 2, cy + h / 2 - 1 - y, r - dx, 1, K_BG);
        cv_rect(cx + w / 2 - (r - dx), cy + h / 2 - 1 - y, r - dx, 1, K_BG);
    }
}
#endif

TOY(toy_pumppad, "pumppad", "Pump Pad", "oval", ROLE_CHORDS, HUE_PINK, 30, 0, "A held chord that ducks on every kick and swells back.", pumppad_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef DEPTH
#undef RECOVERY
#undef COLOUR
#undef SPREAD
