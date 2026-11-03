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
typedef struct { int seen, last, q_due[16], q_n[16], nq; } shadow_t;
#define WAY 0
#define GAP 1
#define DELAY 2
#define THIN 3

static void F(make)(toy_t *t)
{
    shadow_t *s = ST(shadow_t);
    add_pick(t, "Mode", 2, 0, SH_WAY_N, "Play along with the lead, or after it");
    add_pick(t, "Interval", 3, 0, SH_GAP_N, "How far below the lead it sings");
    add_pick(t, "Delay", 3, 1, SH_DELAY_N, "In Echo, how long after the lead");
    add(t, "Thin", 0, 10, 2, "The chance that a note is left out");
    s->seen = S.leads;
    s->last = -2 * BAR;
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
}

static void F(on_step)(toy_t *t)
{
    shadow_t *s = ST(shadow_t);
    int st = t->steps, heard = 0, hn = 0;
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

TOY(toy_shadow, "shadow", "Shadow", "shadow", ROLE_LEAD, HUE_BLUE, 0, 1,
    "A second line that follows the band's lead, below it or after it.",
    shadow_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), 0, 0, 0);
#undef WAY
#undef GAP
#undef DELAY
#undef THIN
