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

#ifdef WB_PICTURES
/* the picture: a bar for each seat, white while it plays, dark while it is cut, grey when empty, its
 * job written under; the noise swell as a line that climbs beside the bars into the cut */
static void F(draw)(toy_t *t)
{
    blackout_t *s = ST(blackout_t);
    const uint8_t *roles = F(roles)();
    int i, n = S.band ? SEATS : 4, y0 = 18, y1 = 104, line = s->line ? (s->line - 26) * 3 / 4 : 0;
    for (i = 0; i < n; i++) {
        int x = pic_col(i, n, 9), w = n > 4 ? 20 : 33;
        uint16_t c = S.cuts[i] ? K_PANEL : roles[i] ? K_TEXT : K_SAND;
        cv_rect(x - w / 2, y0, w, y1 - y0, c);
        if (line) {
            cv_rect(x - w / 2 - 6, line, 5, 2, pic_hue);
            cv_rect(x + w / 2 + 1, line, 5, 2, pic_hue);
        }
        cv_text(x - 14, y1 + 4, &FONT_XS, roles[i] == ROLE_DRUMS ? "DRUM" : roles[i] == ROLE_BASS ? "BASS" :
                roles[i] == ROLE_CHORDS ? "CHRD" : roles[i] == ROLE_LEAD ? "LEAD" : roles[i] == ROLE_VOICE ? "VOX" :
                roles[i] == ROLE_TEXTURE ? "TEX" : roles[i] == ROLE_CONDUCTOR ? "CND" : "-", K_SOFT);
    }
}
#endif

TOY(toy_blackout, "blackout", "Blackout", "bars", ROLE_CONDUCTOR, HUE_PURPLE, 0, 1, "Every so many bars the band is cut, drums first, with a noise swell into the silence.", blackout_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), F(close));
#undef EVERY
#undef LONG
#undef DEPTH
#undef RISER
