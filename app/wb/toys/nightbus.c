/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 70 (Witchbox, texture): Night Bus. The city at night: a low hum that follows the chords,
 * cars passing, and rare events: a siren in tune with the chord, a train, a door. */
#include "toy_util.h"
#undef F
#define F(n) nb_##n

typedef struct { int car_x[2], car_y[2], car_v[2], ncars, siren, flash, train, has_train, door; } nightbus_t;
#define TRAFFIC 0
#define EVENTS 1
#define FAR 2
#define HUM 3

static void F(make)(toy_t *t)
{
    add(t, "Traffic", 0, 10, 5, "How often a car passes");
    add(t, "Events", 0, 10, 4, "How often a siren, train or door is heard");
    add(t, "Distance", 0, 10, 3, "How far away everything sounds");
    add(t, "Hum", 0, 10, 5, "The level of the low hum");
}

static void F(randomise)(toy_t *t)
{
    P(TRAFFIC) = (int16_t)dice_int(11);
    P(EVENTS) = (int16_t)dice_int(11);
    P(FAR) = (int16_t)dice_int(11);
    P(HUM) = (int16_t)dice_int(11);
}

static void F(changed)(toy_t *t)
{
    nightbus_t *s = ST(nightbus_t);
    static int was = 3;             /* Playbox kept this per toy; it only clears the road */
    if (s->ncars && P(FAR) != was)
        s->ncars = 0;
    was = P(FAR);
}

static void F(wipe)(nightbus_t *s)
{
    s->ncars = 0;
    s->has_train = 0;
    s->siren = s->door = 0;
    s->flash = 0;
}

static void F(play_changed)(toy_t *t)
{
    if (!t->playing)
        F(wipe)(ST(nightbus_t));
}

static void F(on_step)(toy_t *t)
{
    nightbus_t *s = ST(nightbus_t);
    int st = t->steps, busy;
    if (st % BAR == 0 && P(HUM))
        toy_note(t, toy_root(t, 0) - 14, 0, 0);    /* the hum, two octaves down */
    if (st % 4 == 0 && s->ncars < 2 && dice_mul_lt(20, P(TRAFFIC))) {
        int far = P(FAR), v = 7 - far / 2, left = dice_lt_q(32768), y = 168 + 40 - far * 2;
        if (s->ncars && s->car_y[0] == y)   /* that lane is taken */
            y -= 12;
        s->car_x[s->ncars] = left ? 298 : 6;
        s->car_y[s->ncars] = y;
        s->car_v[s->ncars] = left ? -v : v;
        s->ncars++;
        toy_note(t, 0, 2, 0);       /* a car goes by */
    }
    if (s->siren && st % 2 == 0) {  /* two notes, back and forth */
        s->siren--;
        s->flash = s->siren % 2 == 1;
        toy_note(t, toy_root(t, 0) + (s->flash ? 4 : 2), 1, 0);
    }
    if (s->has_train && st % 2 == 0)
        toy_note(t, 0, 3, 0);       /* wheels on the rails */
    busy = s->siren || s->has_train || s->door;
    if (st % BAR == 0 && !busy && dice_mul_lt(24, P(EVENTS))) {
        uint32_t r = dice_k();
        if (r < 32768) {
            s->siren = 16;
        } else if (r < 52429u /* r < 0.8 */) {
            s->has_train = 1;
            s->train = -70;
        } else {
            toy_note(t, 0, 4, 0);   /* a door */
            s->door = 400;
        }
    }
}

static void F(frame)(toy_t *t, int ms)
{
    nightbus_t *s = ST(nightbus_t);
    int i, j;
    if (!t->playing)
        return;
    for (i = 0; i < s->ncars; i++)
        s->car_x[i] += s->car_v[i];
    for (i = j = 0; i < s->ncars; i++)
        if (s->car_x[i] >= 6 && s->car_x[i] <= 298) {
            s->car_x[j] = s->car_x[i];
            s->car_y[j] = s->car_y[i];
            s->car_v[j++] = s->car_v[i];
        }
    s->ncars = j;
    if (s->has_train) {
        int x = s->train;
        s->train = x + 6;
        if (x > 330)
            s->has_train = 0;
    }
    if (s->door)
        s->door = imax(0, s->door - ms);
}

TOY(toy_nightbus, "nightbus", "Night Bus", "lamp", ROLE_TEXTURE, HUE_BLUE, 40, 1,
    "The city at night, heard from far off: a hum, passing cars, a siren, a train, a door.",
    nightbus_t, F(make), 0, F(changed), F(randomise), F(on_step), F(play_changed), F(frame), 0, 0);
#undef TRAFFIC
#undef EVENTS
#undef FAR
#undef HUM
