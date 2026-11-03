/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 76 (Witchbox, voice): Screwed Voice. A voice pitched far down, sliding into each long note,
 * with a low murmur in between. */
#include "toy_util.h"
#undef F
#define F(n) sd_##n

static const int8_t SD_DOWN[4] = {-3, -4, -7, -14};
static const char *const SD_DEPTH_N[4] = {"4th", "5th", "1 oct", "2 oct"};
static const char *const SD_DRAWL_N[4] = {"1 bar", "2 bars", "3 bars", "4 bars"};
typedef struct { int n, slide, sing, blink, gape; } screwed_t;
#define DEPTH 0
#define DRAWL 1
#define SLUR 2
#define MUMBLE 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Depth", 4, 2, SD_DEPTH_N, "How far down the voice is pitched");
    add_pick(t, "Drawl", 4, 0, SD_DRAWL_N, "How long each note lasts");
    add(t, "Slur", 0, 10, 5, "How slowly it slides into each note");
    add(t, "Mumble", 0, 10, 3, "The chance of a low murmur in between");
    ST(screwed_t)->n = -7;
}

static void F(randomise)(toy_t *t)
{
    P(DEPTH) = (int16_t)dice_int(4);
    P(DRAWL) = (int16_t)dice_int(4);
    P(SLUR) = (int16_t)dice_int(11);
    P(MUMBLE) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t)
{
    if (!t->playing)                /* show the new depth straight away */
        ST(screwed_t)->n = SD_DOWN[P(DEPTH)];
}

static void F(play_changed)(toy_t *t)
{
    screwed_t *s = ST(screwed_t);
    s->sing = 0;
    s->slide = 0;
}

static void F(on_step)(toy_t *t)
{
    screwed_t *s = ST(screwed_t);
    int st = t->steps, every;
    if (st % 2)                     /* half speed: every other step only */
        return;
    every = (P(DRAWL) + 1) * BAR;
    if (st % every == 0) {
        s->n = toy_root(t, 0) + SD_DOWN[P(DEPTH)];
        s->slide = P(SLUR) * t->step_ms;
        toy_note(t, s->slide ? s->n - 1 : s->n, 0, 0);     /* start a step below */
        s->sing = 1;
    } else if (st % every == every - 6) {
        s->sing = 0;                /* shut the mouth before the next note */
    } else if (st % 8 == 4 && !s->slide && dice_mul_lt(14, P(MUMBLE))) {
        toy_note(t, s->n - 2 - dice_int(3), 1, 0);          /* a mumble */
        s->blink = 240;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    screwed_t *s = ST(screwed_t);
    int full, step;
    if (s->slide && t->playing) {
        s->slide = imax(0, s->slide - ms);
        if (!s->slide)
            toy_note(t, s->n, 0, 0);    /* the slide arrives on the note */
    }
    if (s->blink)
        s->blink = imax(0, s->blink - ms);
    full = 30 + (imax(0, imin(16, 2 - s->n)) * 3) / 3;
    step = 1 + (10 - P(SLUR)) / 3;
    if (s->sing)
        s->gape = imin(full, s->gape + step);
    else
        s->gape = imax(0, s->gape - 2);
}

#ifdef WB_PICTURES
/* the picture: the voice as an orb that hangs lower in its seal the lower the note is pitched, going
 * from cream to the hue with depth, and fills slowly, with Slur, as the mouth opens for a note and
 * drains before the next; two eye runes above it that shut to rests for a mumble */
static void F(draw)(toy_t *t)
{
    screwed_t *s = ST(screwed_t);
    int low = imax(0, imin(16, 2 - s->n)), full = 30 + low, cy = 62 + low * 2, ex;
    uint16_t c = low >= 11 ? pic_hue : low >= 5 ? K_SOFT : K_TEXT;
    pic_seal(PIC_CX, PIC_CY, 64, 0, 0, K_SAND);
    for (ex = 0; ex < 2; ex++)
        pic_rune(PIC_CX - 18 + ex * 28, 22, s->blink > 0 ? RUNE_REST : RUNE_EYE, c);
    pic_orb(PIC_CX, cy, 24, imin(1000, s->gape * 1000 / full), c, c);
}
#endif

TOY(toy_screwed, "screwed", "Screwed Voice", "face", ROLE_VOICE, HUE_PURPLE, 40, 1, "A voice pitched far down, sliding into each long note, muttering in between.", screwed_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef DEPTH
#undef DRAWL
#undef SLUR
#undef MUMBLE
