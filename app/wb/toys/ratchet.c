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
/* the picture: the bar's hits as blocks; a hit that ratchets splits into its strikes, each lit as it
 * lands */
static void F(draw)(toy_t *t)
{
    ratchet_t *s = ST(ratchet_t);
    static const uint8_t DIV[3] = {2, 3, 4};
    int i, now = t->steps % BAR;
    for (i = 0; i < BAR; i++) {
        int x = 8 + i * 14, kind = i == 0 || i == 10 ? 0 : i == 4 || i == 12 ? 1 : i % 2 == 0 ? 2 : -1;
        int y = kind == 0 ? 96 : kind == 1 ? 64 : 40, h = kind == 0 ? 30 : kind == 1 ? 24 : 14;
        if (kind < 0)
            continue;
        if (i == now && s->left >= 0 && s->voice == kind && pic_ago(t, kind) < 300u && (s->left > 0 || s->gap)) {
            int n = DIV[P(SPEED)], k;
            for (k = 0; k < n; k++)
                cv_rect(x + k * 12 / n, y, 12 / n - 1, h, k < n - s->left ? pic_hue : K_SAND);
        } else
            cv_rect(x, y, 11, h, i == now && t->playing ? pic_hue : K_TEXT);
    }
    pic_bar(t->steps, BAR);
}
#endif

TOY(toy_ratchet, "ratchet", "Ratchet", "ratchet", ROLE_DRUMS, HUE_ORANGE, 10, 3, "A plain beat whose hits now and then ratchet into two, three or four quick strikes.", ratchet_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef RATCHETS
#undef SPEED
#undef WHERE
#undef GHOSTS
