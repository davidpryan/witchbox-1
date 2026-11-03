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

#ifdef WB_PICTURES
/* the picture: the register as a seal: a rune per bit round the ring, cream where set, sand where
 * not, the read head a needle in the hue; the drums' taps as small points on the outer ring */
static void F(draw)(toy_t *t)
{
    turing_t *s = ST(turing_t);
    static const uint8_t LEN[4] = {4, 8, 12, 16};
    int len = LEN[P(LENGTH)], i, v, cx = PIC_CX, cy = PIC_CY, head = s->at % len;
    pic_seal(cx, cy, 64, 8, 3, K_SAND);
    pic_needle(cx, cy, 36, head, len, pic_hue);
    for (i = 0; i < len; i++) {
        int on = (s->reg >> i) & 1u;
        pic_rune_at(cx, cy, 46, i, len, on ? i % 6 : RUNE_REST, i == head ? pic_hue : on ? K_TEXT : K_SAND);
    }
    for (v = 0; v < P(VOICES); v++) {
        int tap = (head + v * len / 4) % len;
        pic_pt(cx, cy, 60, tap, len, 2, pic_ago(t, v) < 120u ? pic_hue : K_SOFT);
    }
}
#endif

TOY(toy_turing, "turing", "Turing Drums", "bits", ROLE_DRUMS, HUE_CYAN, 0, 3, "A shift register goes round and three drums read it. Each bit may flip as it passes: locked, evolving, or new.", turing_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef CHANGE
#undef LENGTH
#undef VOICES
#undef DENSITY
