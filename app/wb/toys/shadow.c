/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 73 (Witchbox, lead): Shadow. A second line that follows whichever lead is playing in the
 * band, a third, sixth or octave below, or echoing it later; alone, the chord's top note. */
#include "toy_util.h"
#undef F
#define F(n) sh_##n

static const uint8_t SH_BELOW[3] = {2, 5, 7};
static const uint8_t SH_WAIT[3] = {8, 16, 32};
static const char *const SH_WAY_N[2] = {"Harmony", "Echo"};
static const char *const SH_GAP_N[3] = {"Third", "Sixth", "Octave"};
static const char *const SH_DELAY_N[3] = {"1/2 bar", "1 bar", "2 bars"};
#define SH_EMPTY (-128)
typedef struct {
    int seen, last, q_due[16], q_n[16], nq;
    int8_t heard[32], mine[32]; uint8_t hkind[32];      /* the picture's lanes: a note a column, or SH_EMPTY */
} shadow_t;
#define WAY 0
#define GAP 1
#define DELAY 2
#define THIN 3

static void F(make)(toy_t *t)
{
    shadow_t *s = ST(shadow_t);
    int k;
    add_pick(t, "Mode", 2, 0, SH_WAY_N, "Play along with the lead, or after it");
    add_pick(t, "Interval", 3, 0, SH_GAP_N, "How far below the lead it sings");
    add_pick(t, "Delay", 3, 1, SH_DELAY_N, "In Echo, how long after the lead");
    add(t, "Thin", 0, 10, 2, "The chance that a note is left out");
    s->seen = S.leads;
    s->last = -2 * BAR;
    for (k = 0; k < 32; k++)
        s->heard[k] = s->mine[k] = SH_EMPTY;
}

static void F(randomise)(toy_t *t)
{
    P(WAY) = (int16_t)dice_int(2);
    P(GAP) = (int16_t)dice_int(3);
    P(DELAY) = (int16_t)dice_int(3);
    P(THIN) = (int16_t)dice_int(6);
}

static void F(changed)(toy_t *t)
{
    if (P(WAY) == 0)
        ST(shadow_t)->nq = 0;       /* no echoes left over in Harmony */
}

static void F(play_changed)(toy_t *t)
{
    shadow_t *s = ST(shadow_t);
    s->nq = 0;
    s->seen = S.leads;
    s->last = -2 * BAR;
}

static void F(sing)(toy_t *t, int n)   /* one of our own notes; we are a lead too, so note the count */
{
    shadow_t *s = ST(shadow_t);
    if (dice_mul_lt(12, P(THIN)))
        return;
    toy_note(t, n, 0, 0);
    s->seen = S.leads;
    s->mine[t->steps % 32] = (int8_t)imax(-127, imin(127, n));
}

static void F(on_step)(toy_t *t)
{
    shadow_t *s = ST(shadow_t);
    int st = t->steps, heard = 0, hn = 0, col = st % 32;
    s->heard[(col + 2) % 32] = s->mine[(col + 2) % 32] = SH_EMPTY;   /* the wipe runs two columns ahead */
    s->heard[col] = s->mine[col] = SH_EMPTY;
    if (S.leads != s->seen) {       /* a lead toy has played */
        s->seen = S.leads;
        s->last = st;
        if (S.has_lead) {
            heard = 1;
            hn = S.lead;
        }
    } else if (st - s->last >= 2 * BAR && st % 4 == 0) {
        heard = 1;
        hn = toy_root(t, 0) + 4;    /* no lead: the chord's top note */
    }
    if (heard) {
        s->heard[col] = (int8_t)imax(-127, imin(127, hn));
        s->hkind[col] = (uint8_t)(S.leads == s->seen && st - s->last >= 2 * BAR);   /* 1: no lead, the chord's top */
        if (P(WAY) == 0) {
            F(sing)(t, hn - SH_BELOW[P(GAP)]);
        } else if (s->nq < 16) {
            s->q_due[s->nq] = st + SH_WAIT[P(DELAY)];
            s->q_n[s->nq++] = hn;
        }
    }
    while (s->nq && s->q_due[0] <= st) {
        int i, n = s->q_n[0];
        for (i = 1; i < s->nq; i++) {
            s->q_due[i - 1] = s->q_due[i];
            s->q_n[i - 1] = s->q_n[i];
        }
        s->nq--;
        F(sing)(t, n);
    }
}

#ifdef WB_PICTURES
/* the picture: two rings interlocked, two bars round each: the lead's notes as points on the left
 * ring (soft where it was only the chord's top), the shadow's own on the right in the hue, a point
 * further out the higher the note; a needle in each for the step; a lead rune over the one, an eye
 * over the other */
static void F(draw)(toy_t *t)
{
    shadow_t *s = ST(shadow_t);
    int lane, col, at = t->steps % 32;
    for (lane = 0; lane < 2; lane++) {
        int cx = lane ? 170 : 70, cy = 78;
        pic_seal(cx, cy, 52, 32, 0, K_SAND);
        pic_needle(cx, cy, 40, at, 32, K_LINE);
        pic_rune(cx - 4, 8, lane ? RUNE_EYE : RUNE_LEAD, lane ? pic_hue : K_TEXT);
        for (col = 0; col < 32; col++) {
            int8_t v = lane ? s->mine[col] : s->heard[col];
            int n;
            if (v == SH_EMPTY) {
                pic_pt(cx, cy, 40, col, 32, 0, K_SAND);
                continue;
            }
            n = imax(0, imin(21, v));
            pic_pt(cx, cy, 22 + n, col, 32, 2, lane ? pic_hue : s->hkind[col] ? K_SOFT : K_TEXT);
        }
    }
}
#endif

TOY(toy_shadow, "shadow", "Shadow", "shadow", ROLE_LEAD, HUE_BLUE, 0, 1, "A second line that follows the band's lead, below it or after it.", shadow_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef WAY
#undef GAP
#undef DELAY
#undef THIN
