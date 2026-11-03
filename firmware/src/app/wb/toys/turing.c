/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 101 (Witchbox IV, drums): Turing Drums. A shift register of sixteen bits goes round; three
 * drums read it at different taps. Each time a bit comes round it may flip (Change), so the pattern
 * is locked, slowly evolving, or new every bar. */
#include "toy_util.h"
#undef F
#define F(n) tg_##n

static const uint8_t TG_LEN[4] = {4, 8, 12, 16};
static const char *const TG_LEN_N[4] = {"4 steps", "8 steps", "12 steps", "16 steps"};
typedef struct { uint32_t reg; int at; } turing_t;
#define CHANGE 0
#define LENGTH 1
#define VOICES 2
#define DENSITY 3

static void F(deal)(toy_t *t)
{
    turing_t *s = ST(turing_t);
    int i;
    s->reg = 0;
    for (i = 0; i < 16; i++)
        if (dice_lt(P(DENSITY), 10))
            s->reg |= 1u << i;
    s->at = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Change", 0, 10, 2, "How often a bit flips as it comes round");
    add_pick(t, "Length", 4, 3, TG_LEN_N, "How long the register is");
    add(t, "Voices", 1, 4, 3, "How many drums read the register");
    add(t, "Density", 0, 10, 5, "How many bits start on");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(CHANGE) = (int16_t)dice_int(11);
    P(LENGTH) = (int16_t)dice_int(4);
    P(VOICES) = (int16_t)(1 + dice_int(4));
    P(DENSITY) = (int16_t)(2 + dice_int(8));
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    turing_t *s = ST(turing_t);
    int len = TG_LEN[P(LENGTH)], v, bit;
    if (s->at >= len)
        s->at = 0;
    bit = (s->reg >> s->at) & 1u;
    if (dice_lt(P(CHANGE), 40))
        s->reg ^= 1u << s->at;                  /* the bit flips as it is written back */
    /* each drum reads its own tap: the kick this step, the snare a quarter round on, the hat half */
    for (v = 0; v < P(VOICES); v++) {
        int tap = (s->at + v * len / 4) % len;
        if ((s->reg >> tap) & 1u) {
            if (v == 0)
                toy_kick(t);
            else
                toy_note(t, 0, v, 0);
        }
    }
    (void)bit;
    s->at++;
}

TOY(toy_turing, "turing", "Turing Drums", "bits", ROLE_DRUMS, HUE_CYAN, 0, 3,
    "A shift register goes round and three drums read it. Each bit may flip as it passes: locked, evolving, or new.",
    turing_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef CHANGE
#undef LENGTH
#undef VOICES
#undef DENSITY
