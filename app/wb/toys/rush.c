/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 62 (Witchbox, drums): Snare Rush. Snare rolls that speed up into the end of a bar, a crash
 * on the downbeat that ends them, a plain backbeat between. */
#include "toy_util.h"
#include "rush_plan.h"
#undef F
#define F(n) ru_##n

static const uint8_t RU_EVERY[3] = {2, 4, 8};
static const char *const RU_EVERY_N[3] = {"2 bars", "4 bars", "8 bars"};
static const char *const RU_SPAN_N[4] = {"1/4 bar", "1/2 bar", "3/4 bar", "1 bar"};
static const char *const RU_BACK_N[2] = {"Off", "On"};
typedef struct { int k, tt, hot, strip, beat; } rush_t;
#define EVERY 0
#define SPAN 1
#define ACCEL 2
#define BACK 3

static void F(make)(toy_t *t)
{
    rush_t *s = ST(rush_t);
    add_pick(t, "Every", 3, 0, RU_EVERY_N, "How often a roll comes");
    param_init(&t->p[t->np++], "Length", 1, 4, 2, 1, RU_SPAN_N, "%d", "How long the roll lasts");
    add(t, "Speed-up", 0, 10, 6, "0 is an even roll; 10 ends 4 times as fast");
    add_pick(t, "Backbeat", 2, 1, RU_BACK_N, "A snare on beats 2 and 4 between rolls");
    s->k = s->hot = s->beat = -1;
}

static void F(randomise)(toy_t *t)
{
    P(EVERY) = (int16_t)dice_int(3);
    P(SPAN) = (int16_t)(1 + dice_int(4));
    P(ACCEL) = (int16_t)dice_int(11);
    P(BACK) = (int16_t)dice_int(2);
}

static void F(changed)(toy_t *t)
{
    rush_t *s = ST(rush_t);
    s->k = s->hot = -1;
}

static void F(calm)(rush_t *s)
{
    s->hot = -1;
    s->beat = -1;
}

static void F(play_changed)(toy_t *t)
{
    rush_t *s = ST(rush_t);
    s->k = -1;
    F(calm)(s);
    s->strip = 0;
}

static void F(on_step)(toy_t *t)
{
    rush_t *s = ST(rush_t);
    int i = t->steps % BAR, every = RU_EVERY[P(EVERY)];
    int last = toy_bar(t) % every == every - 1, begin = BAR - 4 * P(SPAN);
    if (i == 0) {
        F(calm)(s);
        if (s->k >= 0) {
            s->k = -1;              /* a roll has just run into this bar line */
            toy_note(t, 0, 3, 0);   /* the crash */
            s->strip = 130;
        }
    }
    if (last && i == begin) {
        s->k = 0;                   /* frame() plays the roll from here */
        s->tt = 0;
    } else if (s->k < 0 && P(BACK) && i % 8 == 4) {
        F(calm)(s);
        toy_note(t, 0, 1, 0);
        s->beat = i / 8;
    } else if (s->beat >= 0 && i % 4 == 2) {
        F(calm)(s);
    }
}

static void F(frame)(toy_t *t, int ms)
{
    rush_t *s = ST(rush_t);
    const uint16_t *hits = RU_PLAN[P(SPAN) - 1][P(ACCEL)];
    if (s->strip)
        s->strip = imax(0, s->strip - ms);
    if (!t->playing || s->k < 0)
        return;
    while (s->k < 72 && hits[s->k] != 65535 && hits[s->k] * t->step_ms / 1000 <= s->tt) {
        s->hot = s->k;
        toy_note(t, 0, 1, 0);
        s->k++;
    }
    s->tt += ms;
}

#ifdef WB_PICTURES
/* the picture: the bar as a seal of sixteen points, the backbeat's two as snare runes; the roll's
 * hits as ticks crowding closer round the rim as it speeds up, the one sounding in the hue and
 * those gone soft; a needle for the step; both rings flare in the hue when the crash lands */
static int F(fsin)(int a)           /* sine of an angle in 1024ths of a turn, between PIC_SIN's steps */
{
    int i = (a >> 5) & 31, f = a & 31;
    return (PIC_SIN[i] * (32 - f) + PIC_SIN[(i + 1) & 31] * f) / 32;
}
static void F(draw)(toy_t *t)
{
    rush_t *s = ST(rush_t);
    const uint16_t *hits = RU_PLAN[P(SPAN) - 1][P(ACCEL)];
    int k, n, cx = PIC_CX, cy = PIC_CY, at = t->steps % BAR, begin = BAR - 4 * P(SPAN);
    pic_seal(cx, cy, 64, BAR, 0, s->strip > 0 ? pic_hue : K_SAND);
    pic_needle(cx, cy, 40, at, BAR, K_LINE);
    for (k = 0; k < BAR; k++)
        pic_pt(cx, cy, 58, k, BAR, k % 4 == 0 ? 2 : 0, K_SAND);
    for (k = 0; k < 72 && hits[k] != 65535; k++) {
        int a = (begin * 64 + hits[k] * 64 / 1000) & 1023;     /* step begin + hits[k] / 1000, in 1024ths */
        int sx = F(fsin)(a), sy = F(fsin)((a + 256) & 1023), r0 = k == s->hot ? 30 : 38, r1 = 52;
        uint16_t c = k == s->hot ? pic_hue : k < s->hot ? K_SOFT : K_TEXT;
        cv_line(cx + r0 * sx / 100, cy - r0 * sy / 100, cx + r1 * sx / 100, cy - r1 * sy / 100, c);
        cv_line(cx + r0 * sx / 100 + 1, cy - r0 * sy / 100, cx + r1 * sx / 100 + 1, cy - r1 * sy / 100, c);
        if (k == s->hot)
            pic_disc(cx + 54 * sx / 100, cy - 54 * sy / 100, 3, pic_hue);
    }
    for (n = 0; n < 2; n++)
        if (P(BACK))
            pic_rune_at(cx, cy, 24, 4 + n * 8, BAR, RUNE_SNARE, n == s->beat ? pic_hue : K_SOFT);
    pic_disc(cx, cy, 3, K_TEXT);
}
#endif

TOY(toy_rush, "rush", "Snare Rush", "rush", ROLE_DRUMS, HUE_ORANGE, 20, 1, "Snare rolls that speed up into the end of the bar and land on a crash.", rush_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef EVERY
#undef SPAN
#undef ACCEL
#undef BACK
