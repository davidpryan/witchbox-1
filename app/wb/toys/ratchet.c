/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 102 (Witchbox IV, drums): Ratchet. A plain beat whose hits now and then ratchet: one step
 * becomes two, three or four quick strikes. */
#include "toy_util.h"
#undef F
#define F(n) rc_##n

static const uint8_t RC_DIV[3] = {2, 3, 4};
static const char *const RC_SPEED_N[3] = {"Twos", "Threes", "Fours"};
static const char *const RC_WHERE_N[4] = {"Snare", "Hats", "Kick", "Any"};
typedef struct { int voice, left, gap, acc; } ratchet_t;
#define RATCHETS 0
#define SPEED 1
#define WHERE 2
#define GHOSTS 3

static void F(make)(toy_t *t)
{
    add(t, "Ratchets", 0, 10, 4, "How often a hit ratchets");
    add_pick(t, "Speed", 3, 1, RC_SPEED_N, "How many strikes a ratchet has");
    add_pick(t, "Where", 4, 0, RC_WHERE_N, "Which drum ratchets");
    add(t, "Ghosts", 0, 10, 3, "How many quiet extra snares");
}

static void F(randomise)(toy_t *t)
{
    P(RATCHETS) = (int16_t)dice_int(11);
    P(SPEED) = (int16_t)dice_int(3);
    P(WHERE) = (int16_t)dice_int(4);
    P(GHOSTS) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t) { ST(ratchet_t)->left = 0; }

static void F(hit)(toy_t *t, int voice)
{
    if (voice == 0)
        toy_kick(t);
    else
        toy_note(t, 0, voice, 0);
}

static void F(on_step)(toy_t *t)
{
    ratchet_t *s = ST(ratchet_t);
    int i = t->steps % BAR, voice = -1, w = P(WHERE);
    if (i == 0 || i == 10)
        voice = 0;
    else if (i == 4 || i == 12)
        voice = 1;
    else if (i % 2 == 0)
        voice = 2;
    else if (dice_lt(P(GHOSTS), 20))
        voice = 3;
    if (voice < 0)
        return;
    if ((w == 3 || (w == 0 && voice == 1) || (w == 1 && voice == 2) || (w == 2 && voice == 0)) &&
        dice_lt(P(RATCHETS), 11)) {
        int n = RC_DIV[P(SPEED)];
        s->voice = voice;
        s->left = n - 1;
        s->gap = t->step_ms / n;
        s->acc = 0;
    }
    F(hit)(t, voice);
}

static void F(frame)(toy_t *t, int ms)
{
    ratchet_t *s = ST(ratchet_t);
    if (!t->playing || s->left <= 0)
        return;
    s->acc += ms;
    while (s->left > 0 && s->acc >= s->gap) {
        s->acc -= s->gap;
        s->left--;
        F(hit)(t, s->voice);
    }
}

#ifdef WB_PICTURES
/* the picture: the bar as a wheel of sixteen points, kicks big, snares cream, hats small, the one
 * sounding in the hue; a hit that ratchets splits into its strikes just outside the ring, each lit
 * as it lands; the rune at the centre names which drum ratchets */
static void F(draw)(toy_t *t)
{
    ratchet_t *s = ST(ratchet_t);
    static const uint8_t DIV[3] = {2, 3, 4};
    static const uint8_t WHICH[4] = {RUNE_SNARE, RUNE_HAT, RUNE_KICK, RUNE_PERC};
    int i, now = t->steps % BAR, cx = PIC_CX, cy = PIC_CY;
    pic_seal(cx, cy, 64, BAR, 0, K_SAND);
    pic_needle(cx, cy, 46, now, BAR, K_LINE);
    for (i = 0; i < BAR; i++) {
        int kind = i == 0 || i == 10 ? 0 : i == 4 || i == 12 ? 1 : i % 2 == 0 ? 2 : -1;
        int size = kind == 0 ? 5 : kind == 1 ? 4 : 2, lit = i == now && t->playing;
        if (kind < 0) {
            pic_pt(cx, cy, 48, i, BAR, 0, K_SAND);
            continue;
        }
        pic_pt(cx, cy, 48, i, BAR, lit ? size + 2 : size, lit ? pic_hue : kind == 1 ? K_TEXT : K_SOFT);
    }
    if (t->playing && s->gap && (s->left > 0 || pic_ago(t, s->voice) < (uint32_t)s->gap)) {
        int n = DIV[P(SPEED)], k, a = pic_ang(now, BAR);   /* the strikes bead along the needle */
        for (k = 0; k < n; k++) {
            int r = 18 + k * 24 / n;
            pic_disc(pic_px(cx, r, a), pic_py(cy, r, a), 3, k < n - s->left ? pic_hue : K_SAND);
        }
    }
    pic_disc(cx, cy, 12, K_BG);
    pic_rune(cx - 4, cy - 6, WHICH[P(WHERE)], K_TEXT);
}
#endif

TOY(toy_ratchet, "ratchet", "Ratchet", "ratchet", ROLE_DRUMS, HUE_ORANGE, 10, 3, "A plain beat whose hits now and then ratchet into two, three or four quick strikes.", ratchet_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef RATCHETS
#undef SPEED
#undef WHERE
#undef GHOSTS
