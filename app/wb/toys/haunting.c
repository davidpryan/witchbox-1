/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 115 (Witchbox IV, lead): Haunting. A sparse whistled note now and then — and every note comes
 * back a few steps later a step lower, and that one too, a dying chain of echoes that is the tune. */
#include "toy_util.h"
#undef F
#define F(n) hn_##n

#define HN_Q 16
typedef struct { int16_t due[HN_Q]; int8_t deg[HN_Q], life[HN_Q]; int n; } haunting_t;
#define SEEDS 0
#define DELAY 1
#define FALL 2
#define LIFE 3

static void F(make)(toy_t *t)
{
    add(t, "Seeds", 0, 10, 3, "How often a new note is started");
    add(t, "Delay", 1, 8, 3, "Steps before a note comes back");
    add(t, "Fall", 0, 4, 1, "How many scale steps lower it comes back");
    add(t, "Life", 1, 8, 4, "How many times a note comes back");
    ST(haunting_t)->n = 0;
}

static void F(randomise)(toy_t *t)
{
    P(SEEDS) = (int16_t)dice_int(11);
    P(DELAY) = (int16_t)(1 + dice_int(8));
    P(FALL) = (int16_t)dice_int(5);
    P(LIFE) = (int16_t)(1 + dice_int(8));
}

static void F(play_changed)(toy_t *t) { ST(haunting_t)->n = 0; }

static void F(add)(toy_t *t, int deg, int life)
{
    haunting_t *s = ST(haunting_t);
    if (s->n >= HN_Q || life <= 0)
        return;
    s->due[s->n] = (int16_t)P(DELAY);
    s->deg[s->n] = (int8_t)deg;
    s->life[s->n] = (int8_t)life;
    s->n++;
}

static void F(on_step)(toy_t *t)
{
    haunting_t *s = ST(haunting_t);
    int i, j, root = toy_root(t, 0);
    for (i = 0; i < s->n; i++) {
        if (--s->due[i] > 0)
            continue;
        toy_note(t, root + s->deg[i], s->life[i] > 1 ? 0 : 1, 0);   /* the last echo is quiet */
        if (s->life[i] > 1) {
            s->due[i] = (int16_t)P(DELAY);
            s->deg[i] = (int8_t)(s->deg[i] - P(FALL));
            s->life[i]--;
        } else {
            for (j = i + 1; j < s->n; j++) {
                s->due[j - 1] = s->due[j];
                s->deg[j - 1] = s->deg[j];
                s->life[j - 1] = s->life[j];
            }
            s->n--;
            i--;
        }
    }
    if (dice_lt(P(SEEDS), 40)) {
        int deg = 7 + dice_int(8);
        toy_note(t, root + deg, 0, 0);
        F(add)(t, deg - P(FALL), P(LIFE));
    }
}

#ifdef WB_PICTURES
/* the picture: every note as a dot that drops a step each time it comes back, fading as its life runs
 * out: chains of ghosts falling */
static void F(draw)(toy_t *t)
{
    haunting_t *s = ST(haunting_t);
    int i, root = toy_root(t, 0);
    for (i = 0; i < s->n; i++) {
        int x = 20 + i * 200 / HN_Q + (HN_Q - i) * 3, y = 124 - (s->deg[i] + 7) * 5, life = s->life[i];
        int k;
        pic_disc(x, y, 3, life > 2 ? K_TEXT : life > 1 ? K_SOFT : K_SAND);
        for (k = 1; k < life && k < 4; k++)
            pic_disc(x, y + k * P(FALL) * 5 + k * 2, 1, K_SAND);
    }
    if (pic_ago(t, 0) < 120u)
        pic_disc(224, 124 - 14 * 5, 5, pic_hue);
    (void)root;
}
#endif

TOY(toy_haunting, "haunting", "Haunting", "ghost", ROLE_LEAD, HUE_GREY, 0, 3, "A whistled note now and then, and every note comes back lower a few steps later, and that one too.", haunting_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef SEEDS
#undef DELAY
#undef FALL
#undef LIFE
