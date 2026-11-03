/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 103 (Witchbox IV, drums): Markov Kit. Each hit chooses the next from a table of chances dealt
 * by the seed: a chain of drums that has habits but no fixed bar. */
#include "toy_util.h"
#undef F
#define F(n) mk_##n

#define MK_N 6                                  /* kick snare hat open rim tom; 6 = a rest */
typedef struct { uint8_t table[MK_N + 1][MK_N + 1]; int last; } markovkit_t;
#define STICKY 0
#define RESTS 1
#define DOWNBEAT 2
#define COLOUR 3

static void F(deal)(toy_t *t)
{
    markovkit_t *s = ST(markovkit_t);
    int a, b;
    for (a = 0; a <= MK_N; a++)
        for (b = 0; b <= MK_N; b++)
            s->table[a][b] = (uint8_t)(1 + dice_int(8));
    s->last = MK_N;
}

static void F(make)(toy_t *t)
{
    add(t, "Sticky", 0, 10, 3, "How much a drum likes to repeat itself");
    add(t, "Rests", 0, 10, 4, "How often the chain rests");
    add(t, "Downbeat", 0, 10, 7, "How surely the kick lands on beat 1");
    add(t, "Colour", 0, 10, 4, "How often the rim and the tom get a turn");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(STICKY) = (int16_t)dice_int(11);
    P(RESTS) = (int16_t)dice_int(11);
    P(DOWNBEAT) = (int16_t)(4 + dice_int(7));
    P(COLOUR) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    markovkit_t *s = ST(markovkit_t);
    int i = t->steps % BAR, b, w[MK_N + 1], total = 0, r, next = MK_N;
    if (i == 0 && dice_lt(P(DOWNBEAT), 11)) {
        next = 0;
    } else {
        for (b = 0; b <= MK_N; b++) {
            w[b] = s->table[s->last][b];
            if (b == s->last)
                w[b] += P(STICKY) * 2;
            if (b == MK_N)
                w[b] += P(RESTS) * 3;
            if (b >= 4 && b < MK_N)
                w[b] = w[b] * P(COLOUR) / 10;
            total += w[b];
        }
        r = dice_int(total);
        for (b = 0; b <= MK_N; b++) {
            if (r < w[b]) {
                next = b;
                break;
            }
            r -= w[b];
        }
    }
    s->last = next;
    if (next == 0)
        toy_kick(t);
    else if (next < MK_N)
        toy_note(t, 0, next, 0);
}

#ifdef WB_PICTURES
/* the picture: the six drums as runes round a seal, the chain's likelier roads as chords between
 * them, those out of the drum just struck in the hue; that drum sits on a disc that swells as it
 * sounds; a rest is the rune at the centre */
static void F(draw)(toy_t *t)
{
    markovkit_t *s = ST(markovkit_t);
    static const uint8_t RUNE[MK_N] = {RUNE_KICK, RUNE_SNARE, RUNE_HAT, RUNE_PERC, RUNE_CROSS, RUNE_DOWN};
    int a, b, cx = PIC_CX, cy = PIC_CY, hot = pic_ago(t, -1) < 150u;
    pic_seal(cx, cy, 64, MK_N, 0, K_SAND);
    for (a = 0; a < MK_N; a++)
        for (b = 0; b < MK_N; b++)
            if (a != b && s->table[a][b] >= 6 && a != s->last)
                pic_chord(cx, cy, 46, a, b, MK_N, K_LINE);
    if (s->last < MK_N)
        for (b = 0; b < MK_N; b++)
            if (b != s->last && s->table[s->last][b] >= 5)
                pic_chord(cx, cy, 46, s->last, b, MK_N, pic_hue);
    for (a = 0; a < MK_N; a++) {
        int on = a == s->last;
        pic_pt(cx, cy, 46, a, MK_N, on ? (hot ? 11 : 9) : 8, on ? pic_hue : K_PANEL);
        pic_rune_at(cx, cy, 46, a, MK_N, RUNE[a], on ? K_BG : K_TEXT);
    }
    if (s->last == MK_N)
        pic_rune(cx - 4, cy - 6, RUNE_REST, pic_hue);
    else
        pic_disc(cx, cy, 2, K_SAND);
}
#endif

TOY(toy_markovkit, "markovkit", "Markov Kit", "chain", ROLE_DRUMS, HUE_PURPLE, 0, 3, "Each hit chooses the next from a table of chances. A chain of drums with habits but no fixed bar.", markovkit_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef STICKY
#undef RESTS
#undef DOWNBEAT
#undef COLOUR
