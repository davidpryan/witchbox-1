/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 57 (Witchbox, lead): Bitcrush Arp. A square-wave arpeggio up and down the chord, fast
 * enough to blur; now and then the last three notes repeat in place. */
#include "toy_util.h"
#undef F
#define F(n) ba_##n

#define BA_N 16
#define BA_SLICE 3
static const uint8_t BA_STAIR[8] = {0, 1, 2, 3, 4, 3, 2, 1};
static const uint8_t BA_DIV[3] = {1, 2, 4};
static const char *const BA_RATE_N[3] = {"16ths", "32nds", "Faster"};
typedef struct { int root, at, lit, due, wait, again, from; } bitarp_t;
#define RATE 0
#define PULSE 1
#define CRUSH 2
#define GLITCH 3

static void F(make)(toy_t *t)
{
    add_pick(t, "Rate", 3, 1, BA_RATE_N, "How fast the notes run");
    add(t, "Pulse", 1, 10, 6, "The wave, from thin to square");
    add(t, "Crush", 0, 10, 4, "From clean to very rough");
    add(t, "Glitch", 0, 10, 3, "How often a slice repeats");
    ST(bitarp_t)->lit = -1;
}

static void F(randomise)(toy_t *t)
{
    P(RATE) = (int16_t)dice_int(3);
    P(PULSE) = (int16_t)(1 + dice_int(10));
    P(CRUSH) = (int16_t)dice_int(11);
    P(GLITCH) = (int16_t)dice_int(8);
}

static int F(note)(bitarp_t *s, int i)
{
    int8_t tones[12];
    tune_tones(s->root, 0, 20, tones);
    return tones[BA_STAIR[i % 8]];
}

static void F(on_step)(toy_t *t)
{
    bitarp_t *s = ST(bitarp_t);
    int r = toy_root(t, 0);
    if (r != s->root)
        s->root = r;
    if (!s->again && dice_lt(P(GLITCH), 40)) {
        s->from = fmod_(s->at - BA_SLICE, BA_N);
        s->again = BA_SLICE * (2 + dice_int(3));
    }
    s->due = BA_DIV[P(RATE)];        /* this step's notes, timed in frame() */
    s->wait = 0;
}

static void F(play)(toy_t *t)
{
    bitarp_t *s = ST(bitarp_t);
    int i;
    if (s->again) {                 /* glitching: go round the slice again */
        s->again--;
        i = (s->from + fmod_(BA_SLICE - 1 - s->again, BA_SLICE)) % BA_N;
    } else {
        i = s->at;
        s->at = (i + 1) % BA_N;
    }
    s->lit = i;
    toy_note(t, 7 + F(note)(s, i), NONE, 0);
}

static void F(frame)(toy_t *t, int ms)
{
    bitarp_t *s = ST(bitarp_t);
    if (!t->playing || !s->due)
        return;
    s->wait -= ms;
    while (s->due && s->wait <= 0) {
        s->due--;
        F(play)(t);
        s->wait += t->step_ms / BA_DIV[P(RATE)];
    }
}

static void F(play_changed)(toy_t *t)
{
    bitarp_t *s = ST(bitarp_t);
    s->due = s->again = 0;
    if (t->playing)
        s->at = 0;
    else
        s->lit = -1;
}

TOY(toy_bitarp, "bitarp", "Bitcrush Arp", "pixels", ROLE_LEAD, HUE_GO, 20, 0,
    "A crunchy arpeggio so fast it blurs into a chord, with glitchy repeats.",
    bitarp_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef RATE
#undef PULSE
#undef CRUSH
#undef GLITCH
