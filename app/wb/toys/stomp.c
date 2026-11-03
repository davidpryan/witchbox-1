/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 43 (Witchbox, drums): Slow Stomp. A half-speed beat: a long kick on beat 1, a huge clap on
 * beat 3, the kick doubling now and then, and every few bars extra kicks at the end. */
#include "toy_util.h"
#undef F
#define F(n) st_##n

static const char *const FILL_N[2] = {"4 bars", "8 bars"};

typedef struct { int boom, ring; } stomp_t;

#define WEIGHT 0
#define TAIL 1
#define DOUBLES 2
#define FILL 3

static void F(make)(toy_t *t)
{
    add(t, "Weight", 1, 10, 6, "How long the kick booms");
    add(t, "Clap tail", 0, 10, 6, "How long the clap rings on");
    add(t, "Doubles", 0, 10, 3, "How often the kick hits twice");
    add_pick(t, "Fill every", 2, 0, FILL_N, "How often the bar ends with extra kicks");
}

static void F(randomise)(toy_t *t)
{
    P(WEIGHT) = (int16_t)(1 + dice_int(10));
    P(TAIL) = (int16_t)dice_int(11);
    P(DOUBLES) = (int16_t)dice_int(11);
    P(FILL) = (int16_t)dice_int(2);
}

static void F(on_step)(toy_t *t)
{
    stomp_t *s = ST(stomp_t);
    int i = t->steps % BAR;
    int every = P(FILL) == 0 ? 4 : 8;
    int filling = toy_bar(t) % every == every - 1 && i >= 12;
    if (i == 0 || (i == 6 && dice_lt(P(DOUBLES), 12)) || (filling && i % 2 == 0)) {
        toy_kick(t);
        s->boom = 120 + P(WEIGHT) * 70;
    }
    if (i == 8) {
        toy_note(t, 0, 1, 0);       /* the clap */
        s->ring = 150 + P(TAIL) * 160;
    }
}

static void F(frame)(toy_t *t, int ms)
{
    stomp_t *s = ST(stomp_t);
    if (s->boom)
        s->boom = imax(0, s->boom - ms);
    if (s->ring)
        s->ring = imax(0, s->ring - ms);
}

#ifdef WB_PICTURES
/* the picture: a disc that swells with the kick's boom, and a square frame that runs out with the clap's tail */
static void F(draw)(toy_t *t)
{
    stomp_t *s = ST(stomp_t);
    static const uint8_t RING[4] = {42, 48, 54, 60};
    int r = 20, cx = 120, cy = 72;
    if (s->boom)
        r += 12 * s->boom / (120 + P(WEIGHT) * 70);
    if (s->ring) {
        int k = RING[imin(3, s->ring * 4 / (150 + P(TAIL) * 160))];
        cv_rect(cx - k, cy - k, 2 * k, 3, pic_hue);
        cv_rect(cx - k, cy + k - 3, 2 * k, 3, pic_hue);
        cv_rect(cx - k, cy - k, 3, 2 * k, pic_hue);
        cv_rect(cx + k - 3, cy - k, 3, 2 * k, pic_hue);
    }
    pic_disc(cx, cy, r, K_TEXT);
}
#endif

TOY(toy_stomp, "stomp", "Slow Stomp", "vee", ROLE_DRUMS, HUE_PURPLE, 40, 0, "A half-speed beat: a long boom of a kick, then a huge clap.", stomp_t, F(make), 0, 0, F(randomise), F(on_step), 0, F(frame), PIC(F(draw)), 0);
#undef WEIGHT
#undef TAIL
#undef DOUBLES
#undef FILL
