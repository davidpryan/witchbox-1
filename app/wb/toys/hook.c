/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 58 (Witchbox, lead): Hook Machine. Writes a two-bar riff, plays it three times, changes some
 * notes on the fourth pass, and after some rounds writes a new one. */
#include "toy_util.h"
#undef F
#define F(n) hk_##n

#define HK_LEN 32
static const uint8_t HK_REACH[4] = {4, 7, 9, 14};
static const char *const HK_REACH_N[4] = {"Fifth", "Octave", "Tenth", "2 octaves"};
typedef struct { int8_t riff[HK_LEN], alt[HK_LEN], hold[HK_LEN]; int pass, made; } hook_t;
#define BUSY 0
#define REACH 1
#define VARY 2
#define ROUNDS 3

static void F(write)(toy_t *t)
{
    hook_t *s = ST(hook_t);
    int top = HK_REACH[P(REACH)], want = 2 + (P(BUSY) * 7) / 5, n, last = 0, st;
    uint8_t on[HK_LEN] = {0};
    s->made = P(BUSY) * 16 + P(REACH);
    on[0] = 1;
    while (want > 1) {              /* beats are likeliest, then half beats */
        st = dice_int(HK_LEN);
        if (!on[st]) {
            int ok = st % 4 == 0 ? (dice_k(), 1) : st % 2 == 0 ? dice_lt_q(39322u /* rand() < 0.6 */) : dice_lt_q(16384u /* rand() < 0.25 */);
            if (ok) {
                on[st] = 1;
                want--;
            }
        }
    }
    n = 2 * dice_int(3);
    for (st = 0; st < HK_LEN; st++) {
        s->riff[st] = s->alt[st] = NONE;
        s->hold[st] = 0;
        if (on[st]) {
            int d = dice_int(3) + 1;
            n += dice_lt_q(32768) ? d : -d;
            if (n < 0 || n > top)   /* bounce off the ends of its range */
                n = n < 0 ? -n : 2 * top - n;
            if (st % 4 == 0)        /* on the beat it lands on a chord note */
                n = imax(0, imin(top, tune_nearest(n, 0)));
            s->riff[st] = (int8_t)n;
            last = st;
        }
        if (s->hold[last] < 4)      /* a note lasts until the next, a beat at most */
            s->hold[last]++;
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Busy", 1, 10, 5, "How many notes are in the riff");
    add_pick(t, "Range", 4, 1, HK_REACH_N, "How far the riff roams");
    add(t, "Vary", 0, 10, 5, "How much the fourth pass changes");
    add(t, "Repeats", 1, 8, 2, "Rounds before it writes a new riff");
    F(write)(t);
}

static void F(randomise)(toy_t *t)
{
    P(BUSY) = (int16_t)(1 + dice_int(10));
    P(REACH) = (int16_t)dice_int(4);
    P(VARY) = (int16_t)dice_int(11);
    P(ROUNDS) = (int16_t)(1 + dice_int(4));
    ST(hook_t)->made = -1;
}

static void F(changed)(toy_t *t)
{
    if (P(BUSY) * 16 + P(REACH) != ST(hook_t)->made)
        F(write)(t);
}

static void F(change)(toy_t *t)    /* the fourth pass: move some of the notes */
{
    hook_t *s = ST(hook_t);
    int top = HK_REACH[P(REACH)], st;
    for (st = 0; st < HK_LEN; st++)
        if (s->riff[st] != NONE && dice_lt(P(VARY), 12)) {
            int d = dice_int(3) + 1, n;
            n = s->riff[st] + (dice_lt_q(32768) ? d : -d);
            n = imax(0, imin(top, n));
            if (n != s->riff[st])
                s->alt[st] = (int8_t)n;
        }
}

static void F(restore)(hook_t *s)
{
    int st;
    for (st = 0; st < HK_LEN; st++)
        s->alt[st] = NONE;
}

static void F(on_step)(toy_t *t)
{
    hook_t *s = ST(hook_t);
    int st = t->steps % HK_LEN, n;
    if (st == 0) {
        int p = (t->steps / HK_LEN) % 4;
        if (p == 0) {
            F(restore)(s);
            if (t->steps && (t->steps / (4 * HK_LEN)) % P(ROUNDS) == 0)
                F(write)(t);
        } else if (p == 3) {
            F(change)(t);
        }
        s->pass = p;
    }
    n = s->alt[st] != NONE ? s->alt[st] : s->riff[st];
    if (n != NONE)
        toy_note(t, 7 + toy_root(t, 0) + n, NONE, 0);
}

static void F(play_changed)(toy_t *t)
{
    hook_t *s = ST(hook_t);
    if (!t->playing)
        F(restore)(s);
    s->pass = 0;
}

#ifdef WB_PICTURES
/* the picture: the two-bar riff as a figure on a wheel of thirty-two: a point a note, as far out
 * as it is high, the notes joined in order; the fourth pass's changed notes in the hue; the needle
 * on the step, and four small points round the middle for the passes, the one playing lit */
static void F(draw)(toy_t *t)
{
    hook_t *s = ST(hook_t);
    int st, top = HK_REACH[P(REACH)], now = t->steps % HK_LEN, i, cx = PIC_CX, cy = PIC_CY, last = -1;
    pic_seal(cx, cy, 64, HK_LEN, 0, K_SAND);
    for (st = 0; st < HK_LEN; st++) {
        int n = s->alt[st] >= 0 ? s->alt[st] : s->riff[st];
        if (n < 0)
            continue;
        if (last >= 0) {
            int a = pic_ang(last, HK_LEN), b = pic_ang(st, HK_LEN), m = s->alt[last] >= 0 ? s->alt[last] : s->riff[last];
            cv_line(pic_px(cx, 24 + m * 30 / top, a), pic_py(cy, 24 + m * 30 / top, a), pic_px(cx, 24 + n * 30 / top, b), pic_py(cy, 24 + n * 30 / top, b), K_LINE);
        }
        last = st;
    }
    if (t->playing)
        pic_needle(cx, cy, 56, now, HK_LEN, K_SOFT);
    for (st = 0; st < HK_LEN; st++) {
        int n = s->alt[st] >= 0 ? s->alt[st] : s->riff[st];
        if (n < 0)
            pic_pt(cx, cy, 58, st, HK_LEN, 0, K_SAND);
        else
            pic_pt(cx, cy, 24 + n * 30 / top, st, HK_LEN, st == now && t->playing ? 4 : s->hold[st] > 1 ? 3 : 2, s->alt[st] >= 0 ? pic_hue : K_TEXT);
    }
    for (i = 0; i < 4; i++)
        pic_pt(cx, cy, 8, i, 4, 1, i == s->pass && t->playing ? pic_hue : K_SAND);
}
#endif

TOY(toy_hook, "hook", "Hook Machine", "hook", ROLE_LEAD, HUE_RED, 0, 0, "Writes a two-bar riff, plays it three times, then changes it on the fourth.", hook_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef BUSY
#undef REACH
#undef VARY
#undef ROUNDS
