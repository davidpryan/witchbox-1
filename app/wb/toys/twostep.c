/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 41 (Witchbox, drums): Two-Step. A limping garage beat: the kick on beat 1 and once more
 * somewhere odd (Skip), a rim snare on 2 and 4, hats between, ghosts in the gaps, every hit a
 * little early or late by an amount fixed per seed. */
#include "toy_util.h"
#undef F
#define F(n) tw_##n

#define K_NONE 0
#define K_KICK 1
#define K_SNARE 2
#define K_HAT 3
#define K_GHOST 4
#define TW_PITCH 18                        /* pixels from one line to the next */
#define TW_LEAD 6                          /* every hit waits this many pixels' worth of time */
#define DUE_N 8

typedef struct {
    int8_t lean[BAR];                   /* each step's fixed lean, -100 early .. 100 late */
    uint8_t rank[BAR], kind[BAR];
    int16_t hot[BAR];                   /* ms each dot stays swollen (the picture) */
    int due_ms[DUE_N], due_step[DUE_N], due_n;
} twostep_t;

enum { SKIP, SHUFFLE, DRIFT, GHOSTS };

static void F(lay)(toy_t *t)
{
    twostep_t *s = ST(twostep_t);
    int i;
    for (i = 0; i < BAR; i++) {
        if (i % 2 == 0)
            s->kind[i] = K_HAT;
        else
            s->kind[i] = s->rank[i] < P(GHOSTS) ? K_GHOST : K_NONE;
    }
    s->kind[4] = s->kind[12] = K_SNARE;
    s->kind[0] = s->kind[P(SKIP) - 1] = K_KICK;
}

static void F(deal)(toy_t *t)
{
    twostep_t *s = ST(twostep_t);
    int i;
    for (i = 0; i < BAR; i++) {
        s->lean[i] = (int8_t)(dice_int(201) - 100);
        s->rank[i] = (uint8_t)dice_int(12);
    }
    s->lean[0] = 0;                     /* beat 1 stays put: the rest limp round it */
    F(lay)(t);
}

static void F(make)(toy_t *t)
{
    add(t, "Skip", 7, 12, 11, "The step the second kick lands on");
    add(t, "Shuffle", 0, 10, 5, "How late the off-beat hats swing");
    add(t, "Drift", 0, 10, 5, "How far hits sit off the grid");
    add(t, "Ghosts", 0, 10, 4, "How many quiet extra hits");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(SKIP) = (int16_t)(7 + dice_int(6));
    P(SHUFFLE) = (int16_t)dice_int(11);
    P(DRIFT) = (int16_t)dice_int(11);
    P(GHOSTS) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(lay)(t); }

static int F(shift)(toy_t *t, int i)      /* pixels step i's hit sits right (late) of its line */
{
    twostep_t *s = ST(twostep_t);
    int px = floordiv(s->lean[i] * P(DRIFT) * 5, 1000);
    if (s->kind[i] == K_HAT && i % 4 == 2)
        px += P(SHUFFLE) / 2;
    return px;
}

static void F(play_changed)(toy_t *t)
{
    twostep_t *s = ST(twostep_t);
    int i;
    s->due_n = 0;
    for (i = 0; i < BAR; i++)
        s->hot[i] = 0;
}

static void F(on_step)(toy_t *t)
{
    twostep_t *s = ST(twostep_t);
    int i = t->steps % BAR;
    if (s->kind[i] && s->due_n < DUE_N) {   /* on the picture one line's gap is one step long */
        s->due_ms[s->due_n] = floordiv((TW_LEAD + F(shift)(t, i)) * t->step_ms, TW_PITCH);
        s->due_step[s->due_n++] = i;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    twostep_t *s = ST(twostep_t);
    int i;
    for (i = 0; i < BAR; i++)
        if (s->hot[i])
            s->hot[i] = (int16_t)imax(0, s->hot[i] - ms);
    if (!t->playing)
        return;
    for (i = 0; i < s->due_n; i++)
        s->due_ms[i] -= ms;
    while (s->due_n && s->due_ms[0] <= 0) {
        int st = s->due_step[0], kind = s->kind[st];
        for (i = 1; i < s->due_n; i++) {
            s->due_ms[i - 1] = s->due_ms[i];
            s->due_step[i - 1] = s->due_step[i];
        }
        s->due_n--;
        if (kind == K_KICK)
            toy_kick(t);
        else if (kind)
            toy_note(t, 0, kind - 1, 0);   /* 1 rim snare, 2 hat, 3 ghost */
        s->hot[st] = 110;
    }
}

#ifdef WB_PICTURES
/* the picture: a seal with sixteen points, the bar round its rim. A hit sits on its point, kicks
 * big in the hue, snares cream, hats small; the point sounding now swells. The playhead is a
 * needle and a rune at the top names the toy's role */
static void F(draw)(toy_t *t)
{
    twostep_t *s = ST(twostep_t);
    static const uint8_t SIZE[5] = {0, 5, 4, 2, 2};
    int i, at = t->steps % BAR;
    pic_seal(PIC_CX, PIC_CY, 62, BAR, 0, K_SAND);
    pic_needle(PIC_CX, PIC_CY, 48, at, BAR, K_LINE);
    for (i = 0; i < BAR; i++) {
        int kind = s->kind[i];
        uint16_t c;
        if (!kind) {
            pic_pt(PIC_CX, PIC_CY, 48, i, BAR, 0, K_SAND);
            continue;
        }
        c = kind == K_KICK ? pic_hue : kind == K_SNARE ? K_TEXT : K_SOFT;
        pic_pt(PIC_CX, PIC_CY, 48, i, BAR, s->hot[i] ? SIZE[kind] + 2 : SIZE[kind], c);
    }
    pic_disc(PIC_CX, PIC_CY, 4, K_TEXT);
    pic_rune(PIC_CX - 4, PIC_CY - 34, RUNE_KICK, K_TEXT);
}
#endif

TOY(toy_twostep, "twostep", "Two-Step", "steps2", ROLE_DRUMS, HUE_BLUE, 20, 0, "A limping garage beat. The second kick lands somewhere odd, and every hit sits a little off the grid.", twostep_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef K_NONE
#undef K_KICK
#undef K_SNARE
#undef K_HAT
#undef K_GHOST
