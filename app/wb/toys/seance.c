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
/* the picture: the five voicings as the five points of a pentagram, each an orb that fills with the
 * chance of going there next; the one sounding in the hue, and its notes stacked as bars at the
 * centre, swelling as the chord is struck */
static void F(draw)(toy_t *t)
{
    seance_t *s = ST(seance_t);
    int b, k, cx = PIC_CX, cy = PIC_CY, w[SE_N], top = 1, hot = pic_ago(t, -1) < 200u;
    for (b = 0; b < SE_N; b++) {
        w[b] = s->table[s->last][b] + (b == s->last ? P(STICKY) * 2 : 0);
        if (b >= 3)
            w[b] = w[b] * P(CLUSTERS) / 10;
        if (w[b] > top)
            top = w[b];
    }
    pic_seal(cx, cy, 64, SE_N, 2, K_SAND);
    for (b = 0; b < SE_N; b++)
        if (b != s->last)
            pic_chord(cx, cy, 48, s->last, b, SE_N, w[b] * 3 >= top ? K_SOFT : K_LINE);
    for (b = 0; b < SE_N; b++) {
        int a = pic_ang(b, SE_N), x = pic_px(cx, 48, a), y = pic_py(cy, 48, a), on = b == s->last;
        pic_disc(x, y, 10, K_BG);
        pic_orb(x, y, on ? (hot ? 10 : 9) : 8, w[b] * 1000 / top, on ? pic_hue : K_SAND, on ? pic_hue : K_SOFT);
    }
    pic_disc(cx, cy, 16, K_BG);
    for (k = 0; k < 4; k++)
        if (SE_VOICING[s->last][k] != 99)
            cv_rect(cx - (hot ? 10 : 8), cy + 10 - (SE_VOICING[s->last][k] + 7) * 20 / 21, hot ? 20 : 16, 2, hot ? pic_hue : K_TEXT);
}
#endif

TOY(toy_seance, "seance", "Seance", "pad", ROLE_CHORDS, HUE_PURPLE, 0, 3, "A glass pad whose voicing — close, open, wide, a cluster, a bare fifth — is chosen each bar by a chain of chances.", seance_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, PIC(F(draw)), 0);
#undef STICKY
#undef RATE
#undef LATE
#undef CLUSTERS
