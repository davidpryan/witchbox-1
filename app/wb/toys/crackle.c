/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 69 (Witchbox, texture): Crackle. Record noise: vinyl pops, rain hiss, a low rumble, and now
 * and then a skip. (The trembling rumble bar and the rain rolled dice in the picture too.) */
#include "toy_util.h"
#undef F
#define F(n) cr_##n

typedef struct {
    int ang, specks[6], nspecks, drop;
    uint8_t sr[6], sa[6], dx[10], dy[10]; int tremble;   /* the picture: where the specks and streaks are */
} crackle_t;
#define POPS 0
#define RAIN 1
#define RUMBLE 2
#define WEAR 3

static void F(bar)(toy_t *t)       /* the rumble bar's width trembles: one roll */
{
    if (P(RUMBLE) && t->playing)
        ST(crackle_t)->tremble = dice_int(4 + 2 * P(RUMBLE)) * 3 / 4;
}

static void F(make)(toy_t *t)
{
    add(t, "Crackle", 0, 10, 5, "How many pops");
    add(t, "Rain", 0, 10, 4, "How loud the hiss is");
    add(t, "Rumble", 0, 10, 4, "How loud the low noise is");
    add(t, "Wear", 0, 10, 3, "How often the record skips");
}

static void F(randomise)(toy_t *t)
{
    P(POPS) = (int16_t)dice_int(11);
    P(RAIN) = (int16_t)dice_int(11);
    P(RUMBLE) = (int16_t)dice_int(11);
    P(WEAR) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t) { F(bar)(t); }
static void F(opened)(toy_t *t) { F(bar)(t); }
static void F(play_changed)(toy_t *t) { F(bar)(t); }

static void F(on_step)(toy_t *t)
{
    crackle_t *s = ST(crackle_t);
    int i = t->steps % BAR;
    if (i == 0) {                   /* the hiss and rumble are beds: set going each bar */
        if (P(RAIN))
            toy_note(t, 0, 1, 0);
        if (P(RUMBLE))
            toy_note(t, 0, 2, 0);
    }
    if (i % 4 == 0 && dice_lt(P(WEAR), 30)) {
        toy_note(t, 0, 3, 0);       /* a skip: the needle jumps back */
        s->ang -= 1800 / 8;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    crackle_t *s = ST(crackle_t);
    int i, j;
    for (i = 0; i < s->nspecks; i++)
        s->specks[i] -= ms;
    while (s->nspecks && s->specks[0] <= 0) {
        for (i = 1; i < s->nspecks; i++) {
            s->specks[i - 1] = s->specks[i];
            s->sa[i - 1] = s->sa[i];
            s->sr[i - 1] = s->sr[i];
        }
        s->nspecks--;
    }
    j = s->drop = (s->drop + 1) % 10;   /* one rain streak a frame */
    if (!t->playing)
        return;
    s->ang = fmod_(s->ang + ms, 1800);
    s->dx[j] = 0;
    if (j < P(RAIN)) {                  /* a rain streak, left or right of the record */
        int x = 16 + dice_int(66), side = dice_k() < 32768u, y;
        y = 8 + dice_int(150);
        s->dx[j] = (uint8_t)((x + (side ? 206 : 0)) * 3 / 4);
        s->dy[j] = (uint8_t)(y * 3 / 4);
    }
    if (j % 3 == 0)
        F(bar)(t);
    if (s->nspecks < 6 && dice_mul_lt(400, P(POPS) * ms)) {
        toy_note(t, 0, 0, 0);       /* a pop */
        s->sa[s->nspecks] = (uint8_t)(dice_k() >> 11);          /* its angle, in 32nds of a turn */
        s->sr[s->nspecks] = (uint8_t)((43 + dice_int(14)) * 3 / 4);   /* its radius: between the groove and the rim */
        s->specks[s->nspecks++] = 120;
    }
}

#ifdef WB_PICTURES
/* the picture: the record as a seal: a ring, a second ring, and a five-point star turning with it;
 * the pops as sparks on its chords; the rumble as a trembling bar underneath */
static void F(draw)(toy_t *t)
{
    crackle_t *s = ST(crackle_t);
    int cx = PIC_CX, cy = 66, a = s->ang * 32 / 1800, i, w;
    pic_circle(cx, cy, 52, K_TEXT);
    pic_circle(cx, cy, 51, K_TEXT);
    pic_circle(cx, cy, 46, K_SAND);
    for (i = 0; i < 5; i++) {
        int p = (a + i * 32 / 5) & 31, q = (a + (i + 2) * 32 / 5) & 31;
        cv_line(pic_px(cx, 44, p), pic_py(cy, 44, p), pic_px(cx, 44, q), pic_py(cy, 44, q), pic_hue);
    }
    pic_disc(cx, cy, 3, pic_hue);
    for (i = 0; i < s->nspecks; i++) {
        int r = s->sr[i], ang = s->sa[i];
        cv_rect(cx + r * PIC_SIN[ang & 31] / 100 - 1, cy - r * PIC_SIN[(ang + 8) & 31] / 100 - 1, 3, 3, pic_hue);
    }
    for (i = 0; i < 10; i++)
        if (s->dx[i])
            cv_rect(s->dx[i], s->dy[i], 1, 4, K_SAND);
    w = P(RUMBLE) ? 15 + P(RUMBLE) * 10 + s->tremble : 0;
    if (w)
        cv_rect(cx - w / 2, 130, w, 3, K_SOFT);
}
#endif

TOY2(toy_crackle, "crackle", "Crackle", "record", ROLE_TEXTURE, HUE_PURPLE, 40, 1, "A bed of record noise: vinyl pops, rain, a low rumble, and now and then a skip.", crackle_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0, F(opened));
#undef POPS
#undef RAIN
#undef RUMBLE
#undef WEAR
