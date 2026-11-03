/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 108 (Witchbox IV, chords): Seance. A glass pad whose voicing is chosen each bar by a chain of
 * chances: close, open, spread over two octaves, a cluster, or a bare fifth — each tends to follow
 * certain others. */
#include "toy_util.h"
#undef F
#define F(n) se_##n

#define SE_N 5
static const int8_t SE_VOICING[SE_N][4] = {{0, 2, 4, 99}, {0, 4, 9, 99}, {-7, 4, 9, 14}, {0, 1, 2, 99}, {-7, 4, 99, 99}};
static const uint8_t SE_RATE[3] = {16, 8, 4};
static const char *const SE_RATE_N[3] = {"Each bar", "Half bar", "Each beat"};
typedef struct { uint8_t table[SE_N][SE_N]; int last; } seance_t;
#define STICKY 0
#define RATE 1
#define LATE 2
#define CLUSTERS 3

static void F(deal)(toy_t *t)
{
    seance_t *s = ST(seance_t);
    int a, b;
    for (a = 0; a < SE_N; a++)
        for (b = 0; b < SE_N; b++)
            s->table[a][b] = (uint8_t)(1 + dice_int(6));
    s->last = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Sticky", 0, 10, 4, "How much a voicing likes to stay");
    add_pick(t, "Rate", 3, 0, SE_RATE_N, "How often the chord is struck");
    add(t, "Late", 0, 3, 0, "How many steps late it lands");
    add(t, "Clusters", 0, 10, 2, "How often the cluster and the bare fifth come up");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(STICKY) = (int16_t)dice_int(11);
    P(RATE) = (int16_t)dice_int(3);
    P(LATE) = (int16_t)dice_int(4);
    P(CLUSTERS) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    seance_t *s = ST(seance_t);
    int every = SE_RATE[P(RATE)], i = t->steps % BAR, b, w[SE_N], total = 0, r, next = s->last, k, root, hold;
    if (i < P(LATE) || (i - P(LATE)) % every != 0)
        return;
    for (b = 0; b < SE_N; b++) {
        w[b] = s->table[s->last][b] + (b == s->last ? P(STICKY) * 2 : 0);
        if (b >= 3)
            w[b] = w[b] * P(CLUSTERS) / 10;
        total += w[b];
    }
    r = dice_int(total > 0 ? total : 1);
    for (b = 0; b < SE_N; b++) {
        if (r < w[b]) {
            next = b;
            break;
        }
        r -= w[b];
    }
    s->last = next;
    root = toy_root(t, -P(LATE));
    hold = every * t->step_ms * 9 / 10;
    for (k = 0; k < 4; k++)
        if (SE_VOICING[next][k] != 99)
            toy_note(t, root + SE_VOICING[next][k], 0, hold);
}

#ifdef WB_PICTURES
/* the picture: the five voicings as little stacks of notes, the one sounding lit, the chances of
 * going from it to each other as lines of weight */
static void F(draw)(toy_t *t)
{
    seance_t *s = ST(seance_t);
    static const char *const NAME[5] = {"CLOSE", "OPEN", "WIDE", "CLSTR", "FIFTH"};
    int b, k;
    for (b = 0; b < SE_N; b++) {
        int x = pic_col(b, SE_N, 16), w = s->table[s->last][b] + (b == s->last ? P(STICKY) * 2 : 0);
        cv_rect(x - 1, 100, 3, 0 + w * 2, b == s->last ? pic_hue : K_LINE);
        for (k = 0; k < 4; k++)
            if (SE_VOICING[b][k] != 99)
                cv_rect(x - 10, 90 - (SE_VOICING[b][k] + 7) * 3, 20, 2, b == s->last ? pic_hue : K_TEXT);
        cv_text(x - 16, 124, &FONT_XS, NAME[b], b == s->last ? K_TEXT : K_DIM);
    }
}
#endif

TOY(toy_seance, "seance", "Seance", "pad", ROLE_CHORDS, HUE_PURPLE, 0, 3, "A glass pad whose voicing — close, open, wide, a cluster, a bare fifth — is chosen each bar by a chain of chances.", seance_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef STICKY
#undef RATE
#undef LATE
#undef CLUSTERS
