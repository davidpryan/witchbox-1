/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 47 (Witchbox, bass): 808 Slide. A booming bass that plays where the kick plays (a drummer's
 * kicks in the band, its own pattern alone), gliding between chord notes, dropping an octave at
 * the bar's end. glide_ms and boom_ms are for the sound engine. */
#include "toy_util.h"
#undef F
#define F(n) s8_##n

typedef struct {
    int last, glide_ms, boom_ms, ring, gt, y, y0, y1;
    uint8_t trace[240];             /* the picture: the pen's height at every column of the bar, 0 = silent */
    int px, nn;                     /* the pen's column, the newest note's columns */
} slide808_t;

#define GLIDE 0
#define SLIDES 1
#define DECAY 2
#define DROPS 3

static void F(make)(toy_t *t)
{
    slide808_t *s = ST(slide808_t);
    add(t, "Glide", 0, 10, 5, "How long a glide takes, instant to slow");
    add(t, "Slides", 0, 10, 5, "How often a note glides to the next");
    add(t, "Decay", 1, 10, 6, "From a short punch to a long boom");
    add(t, "Drops", 0, 10, 5, "How often the bar ends an octave down");
    s->last = -7;
}

static void F(randomise)(toy_t *t)
{
    P(GLIDE) = (int16_t)dice_int(11);
    P(SLIDES) = (int16_t)dice_int(11);
    P(DECAY) = (int16_t)(1 + dice_int(10));
    P(DROPS) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    slide808_t *s = ST(slide808_t);
    s->ring = s->y = 0;
}

static void F(play)(toy_t *t, int deg, int slide)
{
    slide808_t *s = ST(slide808_t);
    s->last = deg;
    s->glide_ms = (slide && s->y) ? P(GLIDE) * 40 : 0;
    s->y0 = s->glide_ms ? s->y : 100 + deg * 5;     /* the picture's height, not Playbox's pixels */
    s->y1 = 100 + deg * 5;
    s->gt = 0;
    s->ring = s->boom_ms = 100 + P(DECAY) * 110;
    s->nn = 0;
    toy_note(t, deg, 0, 0);
}

static void F(pick)(toy_t *t)
{
    int8_t tones[8];
    int n = tune_tones(toy_root(t, 0), -7, 0, tones);
    int deg = tones[dice_int(n)];
    F(play)(t, deg, dice_lt(P(SLIDES), 10));
}

static void F(on_step)(toy_t *t)
{
    slide808_t *s = ST(slide808_t);
    int i = t->steps % BAR;
    if (i == 14) {                  /* the bar's last note */
        if (dice_lt(P(DROPS), 10))
            F(play)(t, imax(-14, s->last - 7), 1);  /* it falls */
        else if (!S.drummer)
            F(pick)(t);
    } else if (!S.drummer && (i == 0 || i == 6 || i == 10)) {
        if (i == 0) {
            int deg = toy_root(t, 0) - 7;
            F(play)(t, deg, dice_lt(P(SLIDES), 10));
        } else {
            F(pick)(t);
        }
    }
}

static void F(frame)(toy_t *t, int ms)
{
    slide808_t *s = ST(slide808_t);
    if (!t->playing)
        return;
    if (S.drummer && toy_new_kick(t))   /* play where the kick plays */
        F(pick)(t);
    s->ring = imax(0, s->ring - ms);
    s->gt += ms;
    if (!s->ring)
        s->y = 0;
    else if (s->gt < s->glide_ms) {
        int f = s->gt * 256 / s->glide_ms;
        f = f * f * (768 - 2 * f) >> 16;
        s->y = s->y0 + ((s->y1 - s->y0) * f >> 8);
    } else
        s->y = s->y1;
    {                               /* the picture's pen: a column per 1/240 of the bar, wiped ten ahead */
        int to, n, k;
        to = imin(239, ((t->steps % BAR) * t->step_ms + t->acc) * 240 / (BAR * t->step_ms));
        n = (to - s->px + 240) % 240;
        if (n && n <= 120) {
            for (k = 0; k < n + 10; k++)
                s->trace[(s->px + k) % 240] = 0;
            for (k = 0; k < n; k++)
                s->trace[(s->px + k) % 240] = (uint8_t)s->y;
            s->nn = imin(240, s->nn + n);
            s->px = to;
        }
    }
}

#ifdef WB_PICTURES
/* the picture: the bass as a trace across the bar: level where a note holds, a slope where it glides,
 * nothing in the silences; the newest note in colour */
static void F(draw)(toy_t *t)
{
    slide808_t *s = ST(slide808_t);
    int i, x0 = s->px;
    pic_hline(0, 240, 128, K_LINE);
    for (i = 0; i < 240; i++) {
        int y = s->trace[i];
        if (y)
            cv_rect(i, y - 1, 1, 3, i == x0 || (x0 - i + 240) % 240 < s->nn ? pic_hue : K_TEXT);
    }
    if (s->y)
        cv_rect(x0 - 1, s->y - 3, 3, 7, pic_hue);
}
#endif

TOY(toy_slide808, "slide808", "808 Slide", "slide", ROLE_BASS, HUE_RED, 25, 0, "A booming bass that glides from note to note and drops an octave at the end of the bar.", slide808_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef GLIDE
#undef SLIDES
#undef DECAY
#undef DROPS
