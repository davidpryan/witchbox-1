/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 59 (Witchbox, lead): Lost Melody. A sparse tune heard from far away: a short phrase, a long
 * silence, sometimes the phrase answered a fifth lower. */
#include "toy_util.h"
#undef F
#define F(n) lo_##n

static const int8_t LO_PENTA[10] = {0, 2, 3, 4, 6, 7, 9, 10, 11, 13};
static const uint8_t LO_TOWER_W[8] = {38, 30, 44, 34, 40, 30, 38, 28};
static const uint8_t LO_TOWER_H[8] = {96, 138, 70, 152, 110, 166, 84, 124};
typedef struct {
    int gap[5], idx[5], n, i, wait, reply;
    uint8_t ltower[8], lcol[8], lfloor[8]; int lms[8], lmax[8], nl;   /* the picture: the windows lit */
} lost_t;
#define FAR 0
#define GAPS 1
#define DROOP 2
#define ANSWER 3

static int F(floors)(int h) { return (h - 12) / 14; }

static void F(make)(toy_t *t)
{
    add(t, "Distance", 0, 10, 6, "How far away it sounds");
    add(t, "Gaps", 1, 10, 5, "How long the silences are");
    add(t, "Droop", 0, 10, 4, "How much each note sags");
    add(t, "Answer", 0, 10, 6, "How often a phrase is answered");
}

static void F(randomise)(toy_t *t)
{
    P(FAR) = (int16_t)dice_int(11);
    P(GAPS) = (int16_t)(1 + dice_int(6));
    P(DROOP) = (int16_t)dice_int(11);
    P(ANSWER) = (int16_t)dice_int(11);
}

static void F(light)(toy_t *t, int floor)  /* light a window on this floor, in a tower tall enough */
{
    lost_t *s = ST(lost_t);
    int w, h, cols, k, col, i, ms;
    for (;;) {
        k = dice_int(8);
        w = LO_TOWER_W[k];
        h = LO_TOWER_H[k];
        if (F(floors)(h) > floor)
            break;
    }
    cols = (w - 4) / 10;
    col = dice_int(cols);
    ms = 700 + P(FAR) * 260;
    for (i = 0; i < s->nl; i++)                 /* already lit: start it again */
        if (s->ltower[i] == k && s->lcol[i] == col && s->lfloor[i] == floor)
            break;
    if (i == s->nl) {
        if (s->nl >= 8) {
            for (i = 1; i < 8; i++) {
                s->ltower[i - 1] = s->ltower[i]; s->lcol[i - 1] = s->lcol[i]; s->lfloor[i - 1] = s->lfloor[i];
                s->lms[i - 1] = s->lms[i]; s->lmax[i - 1] = s->lmax[i];
            }
            s->nl = 7;
        }
        i = s->nl++;
    }
    s->ltower[i] = (uint8_t)k;
    s->lcol[i] = (uint8_t)col;
    s->lfloor[i] = (uint8_t)imax(0, floor);
    s->lms[i] = s->lmax[i] = ms;
}

static void F(phrase)(toy_t *t)    /* three to five notes that wander by a step or two */
{
    lost_t *s = ST(lost_t);
    int i = dice_int(4) + 3, k, n;
    for (k = 0; k < 4; k++) {       /* try to start on a note of the chord */
        int m = fmod_(LO_PENTA[i] - toy_root(t, 0), 7);
        if (m == 0 || m == 2 || m == 4)
            break;
        i = dice_int(4) + 3;
    }
    n = 3 + dice_int(3);
    s->n = 0;
    for (k = 0; k < n; k++) {
        s->gap[s->n] = 2 + dice_int(3) * 2;
        s->idx[s->n++] = i;
        i = imax(2, imin(9, i + dice_int(5) - 2));
    }
}

static void F(on_step)(toy_t *t)
{
    lost_t *s = ST(lost_t);
    int gap, i;
    if (s->wait > 0) {
        s->wait--;
        return;
    }
    if (s->i >= s->n) {             /* the phrase is over */
        s->i = 0;
        if (!s->reply && s->n && dice_lt(P(ANSWER), 10)) {
            s->reply = 1;
        } else {
            s->reply = 0;
            F(phrase)(t);
        }
    }
    gap = s->gap[s->i];
    i = s->idx[s->i];
    s->i++;
    if (s->reply) {
        toy_note(t, 7 + LO_PENTA[i] - 4, NONE, 0);     /* a fifth lower */
        F(light)(t, i - 2);
    } else {
        toy_note(t, 7 + LO_PENTA[i], NONE, 0);
        F(light)(t, i);
    }
    s->wait = gap - 1;
    if (s->i >= s->n)               /* then the long silence */
        s->wait += P(GAPS) * (3 + dice_int(3));
}

static void F(frame)(toy_t *t, int ms)     /* the picture: the lights dim, may sag a floor, go out */
{
    lost_t *s = ST(lost_t);
    int i, j;
    for (i = 0; i < s->nl; i++) {
        int was = s->lms[i];
        s->lms[i] -= ms;
        if (was * 2 >= s->lmax[i] && s->lms[i] * 2 < s->lmax[i] && P(DROOP) > 4 && s->lfloor[i] > 0)
            s->lfloor[i]--;
        if (s->lms[i] <= 0) {
            for (j = i + 1; j < s->nl; j++) {
                s->ltower[j - 1] = s->ltower[j]; s->lcol[j - 1] = s->lcol[j]; s->lfloor[j - 1] = s->lfloor[j];
                s->lms[j - 1] = s->lms[j]; s->lmax[j - 1] = s->lmax[j];
            }
            s->nl--;
            i--;
        }
    }
}

static void F(play_changed)(toy_t *t)
{
    lost_t *s = ST(lost_t);
    if (t->playing) {
        s->i = s->n;
        s->wait = 0;
        s->reply = 1;               /* so the first thing heard is a new phrase */
    }
}

#ifdef WB_PICTURES
/* the picture: a skyline of dark towers; a window lights for each note, higher floors for higher
 * notes, dims, and goes out; a droop sags it a floor */
static void F(draw)(toy_t *t)
{
    lost_t *s = ST(lost_t);
    static const uint8_t X[8] = {4, 36, 61, 97, 125, 158, 184, 218};
    int k, i;
    for (k = 0; k < 8; k++) {
        int w = LO_TOWER_W[k] * 3 / 4, h = LO_TOWER_H[k] * 3 / 4, cols = (LO_TOWER_W[k] - 4) / 10, f, c;
        cv_rect(X[k], 144 - h, w, h, RGB(0, 0, 0));
        for (f = 0; f < F(floors)(LO_TOWER_H[k]); f++)
            for (c = 0; c < cols; c++)
                cv_rect(X[k] + (w - cols * 7 + 3) / 2 + c * 7, 144 - 13 - f * 10, 4, 6, K_SAND);
    }
    for (i = 0; i < s->nl; i++) {
        int k = s->ltower[i], w = LO_TOWER_W[k] * 3 / 4, cols = (LO_TOWER_W[k] - 4) / 10;
        int x = X[k] + (w - cols * 7 + 3) / 2 + s->lcol[i] * 7, y = 144 - 13 - s->lfloor[i] * 10;
        cv_rect(x, y, 4, 6, s->lms[i] * 2 < s->lmax[i] ? K_SOFT : pic_hue);
    }
}
#endif

TOY(toy_lost, "lost", "Lost Melody", "dotring", ROLE_LEAD, HUE_PURPLE, 50, 0, "A sparse tune heard from far away. Each phrase may be answered lower down.", lost_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef FAR
#undef GAPS
#undef DROOP
#undef ANSWER
