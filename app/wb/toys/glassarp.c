/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 56 (Witchbox, lead): Glass Arp. A bright bell through the chord's notes, high up, one a
 * step; some dropped, some an octave up, each followed by up to three quieter echoes. */
#include "toy_util.h"
#undef F
#define F(n) ga_##n

#define GA_N 8
static const int8_t GA_UP[GA_N] = {0, 2, 4, 7, 9, 11, 14, 16};
static const int8_t GA_INOUT[GA_N] = {0, 2, 4, 7, 9, 7, 4, 2};
static const char *const GA_SHAPE_N[4] = {"Up", "Down", "In-out", "Random"};
typedef struct {
    int8_t order[GA_N];
    int was, root;
    int ms[GA_N], glow[GA_N], gem[GA_N];
    int e_ms[8], e_i[8], e_n[8], e_left[8], ne;
} glassarp_t;
#define SHAPE 0
#define SKIPS 1
#define LEAP 2
#define ECHO 3

static void F(make)(toy_t *t)
{
    glassarp_t *s = ST(glassarp_t);
    int i;
    add_pick(t, "Shape", 4, 0, GA_SHAPE_N, "The order the notes are played in");
    add(t, "Skips", 0, 10, 2, "How often a note is dropped");
    add(t, "Leap", 0, 10, 2, "How often a note jumps an octave");
    add(t, "Echo", 0, 10, 5, "How many echoes follow each note");
    for (i = 0; i < GA_N; i++)
        s->order[i] = GA_UP[i];
    s->was = -1;
}

static void F(randomise)(toy_t *t)
{
    P(SHAPE) = (int16_t)dice_int(4);
    P(SKIPS) = (int16_t)dice_int(6);
    P(LEAP) = (int16_t)dice_int(7);
    P(ECHO) = (int16_t)dice_int(11);
    ST(glassarp_t)->was = -1;       /* a Random shape is dealt again */
}

static void F(changed)(toy_t *t)
{
    glassarp_t *s = ST(glassarp_t);
    int sh = P(SHAPE), i;
    if (sh == s->was)
        return;
    s->was = sh;
    for (i = 0; i < GA_N; i++)
        s->order[i] = sh == 3 ? GA_UP[dice_int(GA_N)] : sh == 0 ? GA_UP[i] : sh == 1 ? GA_UP[GA_N - 1 - i] : GA_INOUT[i];
}

static void F(light)(glassarp_t *s, int i, int glow)
{
    s->glow[i] = glow;
    s->ms[i] = 110;
}

static void F(on_step)(toy_t *t)
{
    glassarp_t *s = ST(glassarp_t);
    int r = toy_root(t, 0), i, n, many;
    if (r != s->root)
        s->root = r;
    if (dice_lt(P(SKIPS), 14))
        return;
    i = t->steps % GA_N;
    n = 7 + r + s->order[i];
    s->gem[i] = dice_lt(P(LEAP), 14);
    if (s->gem[i])
        n += 7;
    toy_note(t, n, NONE, 0);
    F(light)(s, i, 1);
    many = (P(ECHO) + 2) / 4;       /* 0 to 3 echoes */
    if (many && s->ne < 8) {
        s->e_ms[s->ne] = 3 * t->step_ms;
        s->e_i[s->ne] = i;
        s->e_n[s->ne] = n;
        s->e_left[s->ne++] = many;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    glassarp_t *s = ST(glassarp_t);
    int i, j;
    for (i = 0; i < GA_N; i++)
        if (s->ms[i]) {
            s->ms[i] = imax(0, s->ms[i] - ms);
            if (!s->ms[i])
                s->glow[i] = 0;
        }
    if (!t->playing)
        return;
    for (i = 0; i < s->ne; i++) {
        s->e_ms[i] -= ms;
        if (s->e_ms[i] <= 0) {
            toy_note(t, s->e_n[i], 1, 0);       /* voice 1: the quieter echo */
            if (s->glow[s->e_i[i]] != 1)
                F(light)(s, s->e_i[i], 2);
            s->e_left[i]--;
            s->e_ms[i] += 3 * t->step_ms;
        }
    }
    for (i = j = 0; i < s->ne; i++)
        if (s->e_left[i] > 0) {
            s->e_ms[j] = s->e_ms[i];
            s->e_i[j] = s->e_i[i];
            s->e_n[j] = s->e_n[i];
            s->e_left[j++] = s->e_left[i];
        }
    s->ne = j;
}

static void F(play_changed)(toy_t *t) { ST(glassarp_t)->ne = 0; }

#ifdef WB_PICTURES
/* the picture: the run as a figure on the seal: eight points, one a step, each as far out as its
 * note is high, joined in the order they are played (a spiral for Up, a lemon for In-out, a
 * tangle for Random); a point flares in the hue as it sounds, softer for an echo, with a spark
 * beyond it when the note leaps an octave; the needle on the step */
static void F(draw)(toy_t *t)
{
    glassarp_t *s = ST(glassarp_t);
    int cx = PIC_CX, cy = PIC_CY, i, now = t->steps % GA_N;
    pic_seal(cx, cy, 64, GA_N, 0, K_SAND);
    for (i = 0; i < GA_N; i++) {
        int j = (i + 1) % GA_N, a = pic_ang(i, GA_N), b = pic_ang(j, GA_N);
        int ra = 22 + s->order[i] * 2, rb = 22 + s->order[j] * 2;
        cv_line(pic_px(cx, ra, a), pic_py(cy, ra, a), pic_px(cx, rb, b), pic_py(cy, rb, b), K_LINE);
    }
    pic_needle(cx, cy, 58, now, GA_N, K_LINE);
    for (i = 0; i < GA_N; i++) {
        int r = 22 + s->order[i] * 2;
        uint16_t c = s->glow[i] == 1 ? pic_hue : s->glow[i] == 2 ? K_SOFT : K_TEXT;
        pic_pt(cx, cy, r, i, GA_N, s->glow[i] == 1 ? 5 : 3, c);
        if (s->gem[i])
            pic_pt(cx, cy, r + 9, i, GA_N, 1, c);
    }
}
#endif

TOY(toy_glassarp, "glassarp", "Glass Arp", "diamond", ROLE_LEAD, HUE_CYAN, 40, 0, "A bright bell running through the chord, with skipped notes, leaps and echoes.", glassarp_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef SHAPE
#undef SKIPS
#undef LEAP
#undef ECHO
