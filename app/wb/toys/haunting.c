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
/* the picture: the scale as a seal, a note's pitch its angle; every echo waiting to come back is a
 * point on the ring, with the ghosts it has still to become stepping inwards and lower along a
 * chord, fading as its life runs out; the seal flares in the hue as a note sounds */
static void F(draw)(toy_t *t)
{
    haunting_t *s = ST(haunting_t);
    int i, k, cx = PIC_CX, cy = PIC_CY, hot = pic_ago(t, -1) < 150u;
    pic_seal(cx, cy, 64, 8, 0, hot ? K_SOFT : K_SAND);
    for (i = 0; i < 8; i++)
        pic_pt(cx, cy, 50, i, 8, 0, K_SAND);
    for (i = 0; i < s->n; i++) {
        int life = s->life[i], a = (s->deg[i] + 14) * 2 & 31, px = pic_px(cx, 50, a), py = pic_py(cy, 50, a);
        for (k = 1; k < life && k < 5; k++) {
            int b = (s->deg[i] - k * P(FALL) + 14) * 2 & 31, r = 50 - k * 9;
            int qx = pic_px(cx, r, b), qy = pic_py(cy, r, b);
            cv_line(px, py, qx, qy, K_LINE);
            pic_disc(qx, qy, 1, K_SAND);
            px = qx;
            py = qy;
        }
        pic_pt(cx, cy, 50, (s->deg[i] + 14) * 2 & 31, 32, life > 2 ? 3 : 2, life > 2 ? K_TEXT : life > 1 ? K_SOFT : K_SAND);
    }
    pic_disc(cx, cy, hot ? 6 : 3, hot ? pic_hue : K_SAND);
    pic_rune(cx - 4, cy - 24, RUNE_VOICE, K_TEXT);
}
#endif

TOY(toy_haunting, "haunting", "Haunting", "ghost", ROLE_LEAD, HUE_GREY, 0, 3, "A whistled note now and then, and every note comes back lower a few steps later, and that one too.", haunting_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef SEEDS
#undef DELAY
#undef FALL
#undef LIFE
