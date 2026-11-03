/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 45 (Witchbox, drums): Limp. A drunk kick and snare: the kick rushes, the snare drags, a
 * lighter kick follows each snare, and now and then a hit goes missing. */
#include "toy_util.h"
#undef F
#define F(n) lp_##n

#define DUE_N 8
static const int SWING[3] = {500, 667, 600};
static const char *const GRID_N[3] = {"Straight", "Triplet", "Quint"};

typedef struct {
    int due_ms[DUE_N], due_v[DUE_N], due_n;
    int at, to, hold, left, hot;
} limp_t;

#define DRAG 0
#define RUSH 1
#define GRID 2
#define STUMBLE 3

static void F(make)(toy_t *t)
{
    add(t, "Drag", 0, 10, 6, "How late the snare is");
    add(t, "Rush", 0, 10, 4, "How early the kick is");
    add_pick(t, "Swing grid", 3, 1, GRID_N, "Where the in-between kick falls");
    add(t, "Stumble", 0, 10, 2, "How often a hit is dropped");
}

static void F(randomise)(toy_t *t)
{
    P(DRAG) = (int16_t)dice_int(11);
    P(RUSH) = (int16_t)dice_int(11);
    P(GRID) = (int16_t)dice_int(3);
    P(STUMBLE) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    limp_t *s = ST(limp_t);
    s->due_n = 0;
    s->to = s->at = s->hold = s->left = s->hot = 0;
}

static void F(send)(toy_t *t, int ms, int voice)
{
    limp_t *s = ST(limp_t);
    if (!dice_lt(P(STUMBLE), 25) && s->due_n < DUE_N) {     /* else it stumbles: no hit */
        s->due_ms[s->due_n] = ms;
        s->due_v[s->due_n++] = voice;
    }
}

static void F(sort)(limp_t *s)      /* Python's list sort on [ms, voice] */
{
    int i, j;
    for (i = 1; i < s->due_n; i++) {
        int m = s->due_ms[i], v = s->due_v[i];
        for (j = i; j > 0 && (s->due_ms[j - 1] > m || (s->due_ms[j - 1] == m && s->due_v[j - 1] > v)); j--) {
            s->due_ms[j] = s->due_ms[j - 1];
            s->due_v[j] = s->due_v[j - 1];
        }
        s->due_ms[j] = m;
        s->due_v[j] = v;
    }
}

static void F(on_step)(toy_t *t)
{
    limp_t *s = ST(limp_t);
    int beat, beat_ms, early, late;
    if (t->steps % 4)
        return;
    beat = t->steps / 4 % 4;
    beat_ms = 4 * t->step_ms;
    early = P(RUSH) * beat_ms / 40;
    late = P(DRAG) * beat_ms / 40;
    if (t->steps == 0)
        F(send)(t, 0, 0);           /* nothing came before the first kick */
    if (beat % 2) {                 /* a snare beat; a kick comes next */
        int gap = beat_ms - early - late;
        F(send)(t, late + SWING[P(GRID)] * gap / 1000, 0);
        F(send)(t, beat_ms - early, 0);
        s->to = -1000;
        s->hold = late;
        s->left = beat_ms - early - late;
    } else {                        /* a kick beat; a snare comes next */
        F(send)(t, beat_ms + late, 1);
        s->to = 1000;
        s->hold = 0;
        s->left = beat_ms;
    }
    F(sort)(s);
}

static void F(frame)(toy_t *t, int ms)
{
    limp_t *s = ST(limp_t);
    int i;
    if (s->hot)
        s->hot = imax(0, s->hot - ms);
    if (t->playing) {
        for (i = 0; i < s->due_n; i++)
            s->due_ms[i] -= ms;
        while (s->due_n && s->due_ms[0] <= 0) {
            int v = s->due_v[0];
            for (i = 1; i < s->due_n; i++) {
                s->due_ms[i - 1] = s->due_ms[i];
                s->due_v[i - 1] = s->due_v[i];
            }
            s->due_n--;
            if (v)
                toy_note(t, 0, 1, 0);
            else
                toy_kick(t);
            s->hot = 110;
        }
        if (s->hold > 0) {
            s->hold -= ms;
        } else if (s->left > 0) {
            int go = imin(ms, s->left);
            s->at += floordiv((s->to - s->at) * go, s->left);
            s->left -= go;
        }
    }
}

#ifdef WB_PICTURES
/* the picture: a seal with the kick's rune at its left point and the snare's at its right; a thick
 * needle swings between them, dragging and rushing as the beat does, in the hue when a hit lands */
static void F(draw)(toy_t *t)
{
    limp_t *s = ST(limp_t);
    int cx = PIC_CX, cy = PIC_CY, a = (s->at * 8 / 1000 + 32) & 31, x, y, i;
    uint16_t c = s->hot ? pic_hue : K_TEXT;
    pic_seal(cx, cy, 64, 4, 0, K_SAND);
    for (i = 0; i < 4; i++)
        pic_pt(cx, cy, 58, i, 4, 2, K_SAND);
    pic_rune_at(cx, cy, 48, 3, 4, RUNE_KICK, s->hot && s->at < 0 ? pic_hue : K_TEXT);
    pic_rune_at(cx, cy, 48, 1, 4, RUNE_SNARE, s->hot && s->at > 0 ? pic_hue : K_TEXT);
    x = pic_px(cx, 40, a);
    y = pic_py(cy, 40, a);
    for (i = -1; i <= 1; i++)
        cv_line(cx + i, cy, x + i, y, c);
    pic_disc(x, y, 5, c);
    pic_disc(cx, cy, 4, K_TEXT);
}
#endif

TOY(toy_limp, "limp", "Limp", "metro", ROLE_DRUMS, HUE_PINK, 30, 0, "A drunk kick and snare. The snare drags, the kick rushes, and now and then a hit goes missing.", limp_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef DRAG
#undef RUSH
#undef GRID
#undef STUMBLE
