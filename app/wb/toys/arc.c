/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 80 (Witchbox, conductor): Arc. The song's shape: intro, build, drop, break, drop, outro, each
 * seat's intensity following the line (drums all the way, bass and chords less, leads only when loud). */
#include "toy_util.h"
#undef F
#define F(n) ar_##n

static const uint8_t AR_SHAPES[3][6] = {{15, 35, 70, 50, 100, 40}, {30, 60, 100, 25, 100, 20}, {70, 30, 90, 35, 100, 40}};
static const uint8_t AR_BARS[3] = {4, 8, 16};
static const uint8_t AR_ALONE[SEATS] = {ROLE_DRUMS, ROLE_BASS, ROLE_CHORDS, ROLE_LEAD};
static const char *const AR_SHAPE_N[3] = {"Slow burn", "Two drops", "Waves"};
static const char *const AR_SIZE_N[3] = {"4 bars", "8 bars", "16 bars"};
static const char *const AR_LOOP_N[2] = {"Once", "Again"};
#define AR_SW 40
#define AR_GW 240
#define AR_YLO 176
#define AR_YHI 48
typedef struct { uint8_t ys[AR_GW + 1]; int made; } arc_t;
#define SHAPE 0
#define SIZE 1
#define CONTRAST 2
#define LOOP 3

static void F(line)(toy_t *t)
{
    arc_t *s = ST(arc_t);
    int k = P(CONTRAST), lv[6], x;
    for (x = 0; x < 6; x++)
        lv[x] = 100 - (100 - AR_SHAPES[P(SHAPE)][x]) * k / 10;
    for (x = 0; x <= AR_GW; x++) {
        int i = imax(0, imin(4, floordiv(x - AR_SW / 2, AR_SW)));
        int f = imax(0, imin(AR_SW, x - AR_SW / 2 - i * AR_SW));
        int v = lv[i] + floordiv((lv[i + 1] - lv[i]) * f, AR_SW);
        s->ys[x] = (uint8_t)(AR_YLO - (AR_YLO - AR_YHI) * v / 100);
    }
    s->made = P(SHAPE) * 16 + k;
}

static void F(make)(toy_t *t)
{
    add_pick(t, "Shape", 3, 1, AR_SHAPE_N, "How the song rises and falls");
    add_pick(t, "Section", 3, 0, AR_SIZE_N, "How long each section lasts");
    add(t, "Contrast", 0, 10, 7, "How far quiet and loud are apart");
    add_pick(t, "Loop", 2, 1, AR_LOOP_N, "Stop after one song, or go round again");
    F(line)(t);
}

static void F(randomise)(toy_t *t)
{
    P(SHAPE) = (int16_t)dice_int(3);
    P(SIZE) = (int16_t)dice_int(3);
    P(CONTRAST) = (int16_t)(3 + dice_int(8));
    P(LOOP) = (int16_t)dice_int(2);
}

static int F(where)(toy_t *t)      /* how far through the song we are, as an x on the graph */
{
    int total = 6 * AR_BARS[P(SIZE)] * BAR, tt = t->playing ? toy_now(t) : 0;
    if (P(LOOP) == 0 && tt >= total)
        return AR_GW;               /* played once: stay at the end */
    return (tt % total) * AR_GW / total;
}

static void F(set)(toy_t *t)       /* give every seat its level for where the song is now */
{
    arc_t *s = ST(arc_t);
    int x = F(where)(t), v = (AR_YLO - s->ys[x]) * 100 / (AR_YLO - AR_YHI), i;
    const uint8_t *roles = S.band ? S.roles : AR_ALONE;
    if (P(LOOP) == 0 && x == AR_GW)
        v = 100;                    /* the song is over: all in */
    for (i = 0; i < SEATS; i++) {
        int job = roles[i], n;
        if (job == ROLE_CONDUCTOR)
            continue;
        if (job == ROLE_BASS)
            n = (v * 3 + 100) / 4;
        else if (job == ROLE_CHORDS)
            n = (v + 100) / 2;
        else if (job == ROLE_LEAD)
            n = v >= 50 ? v : 0;
        else if (job == ROLE_VOICE || job == ROLE_TEXTURE)
            n = 100 - v / 4;
        else
            n = v;                  /* drums, and anything unknown */
        S.levels[i] = (uint8_t)(v >= 100 ? 100 : n);
    }
}

static void F(close)(toy_t *t)
{
    int i;
    (void)t;
    for (i = 0; i < SEATS; i++)
        S.levels[i] = 100;
}

static void F(play_changed)(toy_t *t)
{
    if (t->playing)
        F(set)(t);
    else
        F(close)(t);                /* paused: everyone back to full */
}

static void F(changed)(toy_t *t)
{
    if (P(SHAPE) * 16 + P(CONTRAST) != ST(arc_t)->made)
        F(line)(t);
    F(play_changed)(t);
}

static void F(on_step)(toy_t *t)
{
    if (toy_now(t) % BAR == 0)
        F(set)(t);
}

TOY(toy_arc, "arc", "Arc", "graph", ROLE_CONDUCTOR, HUE_PURPLE, 0, 1,
    "The song's shape: intro, build, drop, break, drop, outro, each part following the line.",
    arc_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, 0, F(close));
#undef SHAPE
#undef SIZE
#undef CONTRAST
#undef LOOP
