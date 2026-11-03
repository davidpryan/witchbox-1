/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 72 (Witchbox, lead): Siren. One long note that glides between notes of the chord, resting
 * on each, with a slow wobble. */
#include "toy_util.h"
#undef F
#define F(n) si_##n

#define SI_LO 3
#define SI_HI 17
#define SI_COLS 120
typedef struct { int a, b, tt, moving; uint8_t ys[SI_COLS]; int x, wave; } siren_t;   /* ys, x, wave: the picture */
#define GLIDE 0
#define VIB 1
#define REACH 2
#define HOLD 3

static void F(aim)(toy_t *t, int live)    /* the next note: a chord note within reach, else the nearest */
{
    siren_t *s = ST(siren_t);
    int r = toy_root(t, 0), here = s->b, far = P(REACH), to, n, k;
    int8_t tones[12], near[12];
    n = tune_tones(r, imax(SI_LO, here - far), imin(SI_HI, here + far), tones);
    for (k = 0, to = 0; k < n; k++)
        if (tones[k] != here)
            near[to++] = tones[k];
    if (to)
        to = near[dice_int(to)];
    else
        to = imax(SI_LO, imin(SI_HI, tune_nearest(here, r)));
    s->tt = 0;
    if (to != here) {
        s->a = here;
        s->b = to;
        s->moving = 1;
        if (live)
            toy_note(t, to, NONE, 0);
    }
}

static void F(pen)(toy_t *t, int ms, int live)   /* move the pitch on by ms, and draw a column of the line */
{
    siren_t *s = ST(siren_t);
    int p256, y;
    s->tt += ms;
    if (s->moving) {                /* the pitch now, in 256ths of a scale step: eased from a to b */
        int glide = P(GLIDE) * 2 * t->step_ms, f = imin(256, s->tt * 256 / glide);
        f = f * f * (768 - 2 * f) >> 16;
        p256 = s->a * 256 + (s->b - s->a) * f;
    } else
        p256 = s->b * 256;
#ifdef WB_PICTURES
    s->wave = (s->wave + ms * 32 / 283) & 31;              /* one turn of the vibrato every 283 ms */
    y = 128 - (p256 - SI_LO * 256) * 8 / 256 + P(VIB) * PIC_SIN[s->wave] * 6 / 1000;
    s->ys[s->x] = (uint8_t)imax(1, imin(PH - 1, y));
    s->x = (s->x + 1) % SI_COLS;
#else
    (void)p256; (void)y;
#endif
    if (s->moving) {
        if (s->tt >= P(GLIDE) * 2 * t->step_ms) {
            s->moving = 0;
            s->tt = 0;
        }
    } else if (s->tt >= P(HOLD) * 2 * t->step_ms) {
        F(aim)(t, live);
    }
}

static void F(make)(toy_t *t)
{
    siren_t *s = ST(siren_t);
    int k;
    add(t, "Glide", 1, 10, 5, "How slow each glide is");
    add(t, "Vibrato", 0, 10, 4, "From none to deep");
    add(t, "Reach", 1, 7, 4, "How far each move goes, in notes");
    add(t, "Hold", 1, 10, 4, "How long it rests on each note");
    s->a = s->b = 10;
    for (k = 0; k < 160 - 5; k++)   /* a line to show before anything has played */
        F(pen)(t, 30, 0);
}

static void F(randomise)(toy_t *t)
{
    P(GLIDE) = (int16_t)(1 + dice_int(10));
    P(VIB) = (int16_t)dice_int(11);
    P(REACH) = (int16_t)(1 + dice_int(7));
    P(HOLD) = (int16_t)(1 + dice_int(10));
}

static void F(frame)(toy_t *t, int ms)
{
    if (t->playing)
        F(pen)(t, ms, 1);
}

static void F(play_changed)(toy_t *t)
{
    siren_t *s = ST(siren_t);
    if (t->playing && !s->moving)
        toy_note(t, s->b, NONE, 0);     /* the note it is resting on */
}

#ifdef WB_PICTURES
/* the picture: the pitch as a point gliding round a seal of the fifteen notes it can rest on (the one
 * it is on, or heading for, big in cream); where it has been as a trail that spirals in behind it,
 * the newest in the hue; the vibrato wobbles the point on its way */
static int F(fsin)(int a)           /* sine of an angle in 1024ths of a turn, between PIC_SIN's steps */
{
    int i = (a >> 5) & 31, f = a & 31;
    return (PIC_SIN[i] * (32 - f) + PIC_SIN[(i + 1) & 31] * f) / 32;
}
static void F(draw)(toy_t *t)
{
    siren_t *s = ST(siren_t);
    int i, cx = PIC_CX, cy = PIC_CY, n = SI_HI - SI_LO + 1;
    pic_seal(cx, cy, 64, n, 0, K_SAND);
    for (i = 0; i < n; i++)
        pic_pt(cx, cy, 58, i, n, i + SI_LO == s->b ? 3 : 1, i + SI_LO == s->b ? K_TEXT : K_SAND);
    for (i = SI_COLS - 5; i >= 0; i--) {        /* oldest first, so the newest lies on top */
        int y = s->ys[(s->x - 1 - i + SI_COLS) % SI_COLS], p8 = 128 - y;    /* 8ths of a note above SI_LO */
        int a = (p8 * 1024 / (8 * n)) & 1023, r = 50 - i / 3;
        int x = cx + r * F(fsin)(a) / 100, yy = cy - r * F(fsin)((a + 256) & 1023) / 100;
        if (!y)
            continue;
        if (i < 14)
            pic_disc(x, yy, i == 0 ? 3 : 2, pic_hue);
        else
            cv_rect(x, yy, 2, 2, i < 50 ? K_SOFT : K_SAND);
    }
}
#endif

TOY(toy_siren, "siren", "Siren", "siren", ROLE_LEAD, HUE_PURPLE, 30, 1, "One long note that glides between notes of the chord with a slow wobble.", siren_t, F(make), 0, 0, F(randomise), 0, F(play_changed), F(frame), PIC(F(draw)), 0);
#undef GLIDE
#undef VIB
#undef REACH
#undef HOLD
