/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 44 (Witchbox, drums): Pocket Change. Tiny found sounds scattered as ghost percussion, most
 * likely just before and after beats 2 and 4. */
#include "toy_util.h"
#undef F
#define F(n) pk_##n

#define KEEP 8                      /* how many things are on screen at most */
#define THINGS (KEEP + 2)
static const char *const KIT_N[4] = {"Metal", "Wood", "Paper", "Glass"};
static const uint8_t ODDS[BAR] = {2, 1, 3, 9, 2, 9, 3, 1, 2, 1, 3, 9, 2, 9, 3, 1};

typedef struct {
    int x[THINGS], at[THINGS], fall[THINGS], n;    /* each thing: x, ms so far, ms of fall */
} pocket_t;

#define CLUTTER 0
#define KIT 1
#define LATE 2
#define SCATTER 3

static void F(rest)(toy_t *t)
{
    pocket_t *s = ST(pocket_t);
    static const int X[3] = {120, 168, 204};
    int i;
    s->n = 3;
    for (i = 0; i < 3; i++) {
        s->x[i] = X[i];
        s->at[i] = 9999;
        s->fall[i] = 1;
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Clutter", 1, 10, 5, "How many hits, sparse to busy");
    add_pick(t, "Kit", 4, 0, KIT_N, "What the clicks are made of");
    add(t, "Lateness", 0, 10, 3, "How far hits fall behind the beat");
    add(t, "Scatter", 0, 10, 6, "How widely hits spread left and right");
    F(rest)(t);
}

static void F(randomise)(toy_t *t)
{
    P(CLUTTER) = (int16_t)(1 + dice_int(10));
    P(KIT) = (int16_t)dice_int(4);
    P(LATE) = (int16_t)dice_int(11);
    P(SCATTER) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t)
{
    pocket_t *s = ST(pocket_t);
    s->n = 0;
    if (!t->playing)
        F(rest)(t);
}

static void F(on_step)(toy_t *t)
{
    pocket_t *s = ST(pocket_t);
    int nxt = (t->steps + 1) % BAR;
    /* decide one step ahead, so the thing can land (and sound) on the next step, or a little after */
    if (dice_mul_lt(100, ODDS[nxt] * P(CLUTTER)) && s->n < THINGS) {
        int lag = (int)((dice_k() * (uint32_t)(P(LATE) * t->step_ms)) >> 16) / 12;   /* < 2^31 */
        int m = 6 + P(SCATTER) * 14;
        int x = 160 + ((2 * (int)dice_k() - 65536) * m) / 65536;   /* int() truncates, as C does */
        s->x[s->n] = x;
        s->at[s->n] = 0;
        s->fall[s->n] = t->step_ms + lag;
        s->n++;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    pocket_t *s = ST(pocket_t);
    int i;
    if (!t->playing)
        return;
    if (s->n > KEEP) {              /* the oldest one lying there goes */
        for (i = 1; i < s->n; i++) {
            s->x[i - 1] = s->x[i];
            s->at[i - 1] = s->at[i];
            s->fall[i - 1] = s->fall[i];
        }
        s->n--;
    }
    for (i = 0; i < s->n; i++) {
        int was = s->at[i];
        s->at[i] += ms;
        if (was < s->fall[i] && s->fall[i] <= s->at[i])
            toy_note(t, 0, 3 + P(KIT), 0);      /* it lands: the click */
    }
}

#ifdef WB_PICTURES
/* the picture: a seal with a wide outer ring and a small inner one; each thing is a point that
 * drops in from the outer ring along its own radius, faster and faster, flashes the hue as it
 * lands on the inner ring, and lies there; the steps round the outer ring, bigger where hits
 * are likelier; the kit's rune in the middle, the step a faint needle */
static void F(draw)(toy_t *t)
{
    pocket_t *s = ST(pocket_t);
    static const uint8_t KIT_RUNE[4] = {RUNE_PERC, RUNE_KICK, RUNE_WAVE, RUNE_EYE};
    int cx = PIC_CX, cy = PIC_CY, i;
    pic_circle(cx, cy, 64, K_SAND);
    pic_circle(cx, cy, 60, K_SAND);
    pic_circle(cx, cy, 22, K_SAND);
    pic_needle(cx, cy, 18, t->steps % BAR, BAR, K_LINE);
    for (i = 0; i < BAR; i++)            /* the steps, bigger where a thing is likelier */
        pic_pt(cx, cy, 62, i, BAR, ODDS[i] > 4 ? 1 : 0, K_SAND);
    for (i = 0; i < s->n; i++) {
        int at = s->at[i], fall = s->fall[i], a = (s->x[i] * 32 / 320 + 16) & 31, r;
        uint16_t c;
        if (at < fall)
            r = 60 - 36 * at / fall * at / fall;
        else if (at < fall + 260) {
            int u = at - fall;
            r = 24 + 10 * 4 * u * (260 - u) / (260 * 260);
        } else
            r = 24;
        c = at < fall ? K_SOFT : at < fall + 260 ? pic_hue : K_TEXT;
        pic_disc(pic_px(cx, r, a), pic_py(cy, r, a), 3, c);
    }
    pic_rune(cx - 4, cy - 6, KIT_RUNE[P(KIT)], K_TEXT);
}
#endif

TOY(toy_pocket, "pocket", "Pocket Change", "key", ROLE_DRUMS, HUE_ORANGE, 30, 0, "Keys, coins and clicks dropped around the beat as ghostly percussion.", pocket_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), F(frame), PIC(F(draw)), 0);
#undef CLUTTER
#undef KIT
#undef LATE
#undef SCATTER
