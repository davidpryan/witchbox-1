/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 74 (Witchbox, voice): Ghost Voice. Short sung phrases that come and go at odd moments,
 * starting and ending on a chord note. */
#include "toy_util.h"
#undef F
#define F(n) gh_##n

static const int8_t GH_SHIFT[5] = {-7, -4, 0, 4, 7};
static const char *const GH_PITCH_N[5] = {"-Oct", "-5th", "0", "+5th", "+Oct"};
typedef struct { int todo[4], ntodo, wait, open, to_y, y, h; } ghost_t;
#define PITCH 0
#define PHRASE 1
#define GLIDE 2
#define HAUNT 3

static void F(make)(toy_t *t)
{
    ghost_t *s = ST(ghost_t);
    add_pick(t, "Pitch", 5, 2, GH_PITCH_N, "Moves the voice down or up");
    add(t, "Phrase", 2, 4, 3, "How many notes in each phrase");
    add(t, "Glide", 0, 10, 5, "How slowly it slides between notes");
    add(t, "Haunt", 1, 10, 6, "How often a phrase appears");
    s->to_y = s->y = 123;
    s->h = 10;
}

static void F(randomise)(toy_t *t)
{
    P(PITCH) = (int16_t)dice_int(5);
    P(PHRASE) = (int16_t)(2 + dice_int(3));
    P(GLIDE) = (int16_t)dice_int(11);
    P(HAUNT) = (int16_t)(1 + dice_int(10));
}

static void F(play_changed)(toy_t *t)
{
    ghost_t *s = ST(ghost_t);
    if (!t->playing) {
        s->ntodo = 0;
        s->open = 0;
        s->to_y = 123;
    }
}

static void F(on_step)(toy_t *t)
{
    ghost_t *s = ST(ghost_t);
    if (s->wait) {
        s->wait--;
        return;
    }
    if (s->ntodo) {
        int n = s->todo[0], i;
        for (i = 1; i < s->ntodo; i++)
            s->todo[i - 1] = s->todo[i];
        s->ntodo--;
        toy_note(t, n, 0, 0);
        s->to_y = 123 - imax(-12, imin(12, n - GH_SHIFT[P(PITCH)] - 4)) * 3;
        s->wait = 1 + dice_int(3);  /* notes 2 to 4 steps apart */
        if (!s->ntodo)
            s->wait += 2;           /* the last note hangs on */
    } else if (s->open) {           /* the phrase is over */
        s->open = 0;
        s->to_y = 123;
        s->wait = 3;
    } else if (t->steps % 2 == 0 && dice_mul_lt(30, P(HAUNT))) {
        int root = toy_root(t, 0), k = P(PHRASE), i;
        int n = GH_SHIFT[P(PITCH)] + root + 2 * dice_int(4);    /* a chord note */
        s->todo[0] = n;
        s->ntodo = 1;
        for (i = 1; i < k; i++) {
            n += dice_int(5) - 2;   /* wander a step or two */
            s->todo[s->ntodo++] = i == k - 1 ? tune_nearest(n, root) : n;
        }
        s->open = 14 + 14 * k;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    ghost_t *s = ST(ghost_t);
    int want = s->open ? s->open : 10;
    (void)ms;
    if (s->h != want) {
        int step = want > s->h ? 12 : 6;
        s->h += imax(-step, imin(step, want - s->h));
    }
    if (s->y != s->to_y) {
        int step = 1 + (10 - P(GLIDE)) * 2 / 3;
        s->y += imax(-step, imin(step, s->to_y - s->y));
    }
}

#ifdef WB_PICTURES
/* the picture: a mouth, shut between phrases, that opens wide for a phrase (wider for a longer one),
 * in colour, and drifts up and down with the notes it sings */
static void F(draw)(toy_t *t)
{
    ghost_t *s = ST(ghost_t);
    int h = s->h * 3 / 4, y = (s->y - 26) * 3 / 4 - h / 2, w = 63, r = imin(7, h / 2);
    pic_rrect(120 - w / 2, y, w, h, r, s->open ? pic_hue : K_TEXT);
    if (h >= 18)
        pic_rrect(120 - w / 2 + 9, y + 6, w - 18, h - 12, imin(4, (h - 12) / 2), K_BG);
}
#endif

TOY(toy_ghost, "ghost", "Ghost Voice", "mouth", ROLE_VOICE, HUE_PINK, 40, 1, "Short sung phrases that come and go at odd moments, drifting a step or two.", ghost_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef PITCH
#undef PHRASE
#undef GLIDE
#undef HAUNT
