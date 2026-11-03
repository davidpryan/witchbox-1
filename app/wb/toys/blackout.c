/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 78 (Witchbox, conductor): Blackout. Every so many bars the band is cut, drums first, for a
 * beat to two bars, with a noise swell into the cut; everyone comes back on the bar line. */
#include "toy_util.h"
#undef F
#define F(n) bo_##n

static const uint8_t BO_EVERY[3] = {4, 8, 16};
static const uint8_t BO_LONG[4] = {4, 8, 16, 32};
static const uint8_t BO_ORDER[5] = {ROLE_DRUMS, ROLE_BASS, ROLE_CHORDS, ROLE_LEAD, ROLE_NONE};
static const uint8_t BO_ALONE[SEATS] = {ROLE_DRUMS, ROLE_BASS, ROLE_CHORDS, ROLE_LEAD};
static const char *const BO_EVERY_N[3] = {"4 bars", "8 bars", "16 bars"};
static const char *const BO_LONG_N[4] = {"1 beat", "2 beats", "1 bar", "2 bars"};
static const char *const BO_DEPTH_N[4] = {"1", "2", "3", "All"};
typedef struct { int cutting, line; } blackout_t;
#define EVERY 0
#define LONG 1
#define DEPTH 2
#define RISER 3

static const uint8_t *F(roles)(void) { return S.band ? S.roles : BO_ALONE; }

static void F(let_go)(blackout_t *s)
{
    int i;
    for (i = 0; i < SEATS; i++)
        S.cuts[i] = 0;
    s->cutting = 0;
}

static void F(make)(toy_t *t)
{
    add_pick(t, "Every", 3, 0, BO_EVERY_N, "How often the band is cut");
    add_pick(t, "Length", 4, 1, BO_LONG_N, "How long each cut lasts");
    param_init(&t->p[t->np++], "Depth", 1, 4, 1, 1, BO_DEPTH_N, "%d", "How many parts drop out");
    add(t, "Riser", 0, 10, 6, "The level of the noise swell");
}

static void F(randomise)(toy_t *t)
{
    P(EVERY) = (int16_t)dice_int(3);
    P(LONG) = (int16_t)dice_int(4);
    P(DEPTH) = (int16_t)(1 + dice_int(4));
    P(RISER) = (int16_t)dice_int(11);
}

static void F(close)(toy_t *t) { F(let_go)(ST(blackout_t)); }

static void F(changed)(toy_t *t)
{
    blackout_t *s = ST(blackout_t);
    if (s->cutting)                 /* cut the newly chosen seats instead */
        F(let_go)(s);
}

static void F(play_changed)(toy_t *t)
{
    blackout_t *s = ST(blackout_t);
    if (!t->playing) {
        F(let_go)(s);
        s->line = 0;
    }
}

static void F(cut)(toy_t *t)       /* silence the first Depth seats that may be cut, drums first */
{
    const uint8_t *roles = F(roles)();
    int left = P(DEPTH), j, i;
    for (j = 0; j < 5; j++)
        for (i = 0; i < SEATS; i++)
            if (roles[i] == BO_ORDER[j] && left) {
                S.cuts[i] = 1;
                left--;
            }
    ST(blackout_t)->cutting = 1;
}

static void F(on_step)(toy_t *t)
{
    blackout_t *s = ST(blackout_t);
    int period = BO_EVERY[P(EVERY)] * BAR, cut = BO_LONG[P(LONG)];
    int p = toy_now(t) % period, start = period - cut, rise = start - 8;
    if ((p >= start) != s->cutting) {
        if (s->cutting)
            F(let_go)(s);           /* the bar line: everyone back in */
        else
            F(cut)(t);
    }
    if (p == rise && P(RISER))
        toy_note(t, 0, 0, 0);       /* the noise swell begins */
    if (p >= rise && P(RISER)) {
        int top = 196 - 152 * P(RISER) / 10;
        s->line = 194 - (194 - top) * (p - rise + 1) / (period - rise);
    } else {
        s->line = 0;
    }
}

TOY(toy_blackout, "blackout", "Blackout", "bars", ROLE_CONDUCTOR, HUE_PURPLE, 0, 1,
    "Every so many bars the band is cut, drums first, with a noise swell into the silence.",
    blackout_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, 0, F(close));
#undef EVERY
#undef LONG
#undef DEPTH
#undef RISER
