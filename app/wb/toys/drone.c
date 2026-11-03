/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 50 (Witchbox, bass): Drone. One note held under everything, on the key note, its fifth or
 * both, struck again every four bars, its tone slowly breathing (tone 0..100, for the engine). */
#include "toy_util.h"
#undef F
#define F(n) dr_##n

static const char *const DR_NOTE_N[3] = {"Root", "Fifth", "Both"};
static const char *const DR_OCT_N[3] = {"Low", "Middle", "Both"};

typedef struct { int tone, tt, again; } drone_t;

#define WHICH 0
#define OVER 1
#define BREATHE 2
#define OCTAVE 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Note", 3, 0, DR_NOTE_N, "Which note it holds");
    add(t, "Overtones", 0, 10, 5, "From a pure tone to a rich buzz");
    add(t, "Breathe", 1, 10, 5, "How fast the tone shifts");
    add_pick(t, "Octave", 3, 0, DR_OCT_N, "How deep it sits");
    ST(drone_t)->again = 1;
}

static void F(randomise)(toy_t *t)
{
    P(WHICH) = (int16_t)dice_int(3);
    P(OVER) = (int16_t)dice_int(11);
    P(BREATHE) = (int16_t)(1 + dice_int(10));
    P(OCTAVE) = (int16_t)dice_int(3);
}

static void F(changed)(toy_t *t) { ST(drone_t)->again = 1; }
static void F(play_changed)(toy_t *t) { ST(drone_t)->again = 1; }

static void F(on_step)(toy_t *t)
{
    drone_t *s = ST(drone_t);
    if (s->again || t->steps % (4 * BAR) == 0) {
        static const int8_t octs[3][2] = {{-14, 99}, {-7, 99}, {-14, -7}};
        static const int8_t degs[3][2] = {{0, 99}, {4, 99}, {0, 4}};
        int v = 0, a, b;
        s->again = 0;
        for (a = 0; a < 2 && octs[P(OCTAVE)][a] != 99; a++)
            for (b = 0; b < 2 && degs[P(WHICH)][b] != 99; b++)
                toy_note(t, octs[P(OCTAVE)][a] + degs[P(WHICH)][b], v++, 0);   /* the key's own notes */
    }
}

/* 50 - 50 cos(2 pi x / 65536), in whole numbers: a quarter wave of 64 steps */
static const uint8_t DR_COS[65] = {100, 100, 100, 100, 99, 99, 98, 97, 96, 95, 94, 93, 91, 90, 88, 86,
    85, 83, 81, 79, 77, 74, 72, 70, 67, 65, 63, 60, 58, 55, 52, 50, 47, 45, 42, 40, 37, 35, 33, 30,
    28, 26, 23, 21, 19, 17, 15, 14, 12, 10, 9, 7, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0};

static void F(frame)(toy_t *t, int ms)
{
    drone_t *s = ST(drone_t);
    if (t->playing) {               /* one breath takes 16 seconds down to 3 */
        int q, i;
        s->tt = (s->tt + ms * 65536 / (17500 - P(BREATHE) * 1450)) & 65535;
        q = s->tt >> 15;            /* 0 = rising, 1 = falling */
        i = (s->tt >> 9) & 63;
        s->tone = q == 0 ? 100 - DR_COS[i] : DR_COS[i];   /* 0 at the start, 100 half way through */
    }
}

#ifdef WB_PICTURES
/* the picture: one bar across the middle, fatter for more notes, swelling as the tone opens; the
 * overtones come in above it one by one, shorter and higher */
static void F(draw)(toy_t *t)
{
    drone_t *s = ST(drone_t);
    int tone = t->playing ? s->tone : 0, notes = (P(WHICH) == 2 ? 2 : 1) * (P(OCTAVE) == 2 ? 2 : 1);
    int half = 4 + notes * 2 + tone * 6 / 100, n, k;
    cv_rect(30, 93 - half, 180, 2 * half, K_TEXT);
    n = t->playing ? (P(OVER) * (30 + tone * 70 / 100) + 50) / 100 : 0;
    for (k = 0; k < n && k < 10; k++)
        cv_rect(30 + k * 8, 93 - 27 - k * 7, 180 - k * 16, 2, pic_hue);
}
#endif

TOY(toy_drone, "drone", "Drone", "hbar", ROLE_BASS, HUE_PURPLE, 50, 0, "One note held under everything. It ignores the chords and slowly shifts its tone.", drone_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef WHICH
#undef OVER
#undef BREATHE
#undef OCTAVE
