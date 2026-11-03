/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 83 (Witchbox III, drums): Scrapyard. Three machines for drums — tin, glass and wood — each
 * clanking on a cycle of its own length, so they only line up now and then. */
#include "toy_util.h"
#undef F
#define F(n) scy_##n

static const char *const SC_MESH_N[3] = {"Loose", "Locked", "Odd"};
typedef struct { uint8_t off[3], deg[3]; } scrap_t;
#define TIN 0
#define GLASS 1
#define WOOD 2
#define MESH 3

static void F(deal)(toy_t *t)
{
    scrap_t *s = ST(scrap_t);
    int m;
    for (m = 0; m < 3; m++) {
        s->off[m] = (uint8_t)dice_int(8);
        s->deg[m] = (uint8_t)dice_int(4);
    }
}

static void F(make)(toy_t *t)
{
    add(t, "Tin", 2, 9, 3, "The tin machine's cycle, in steps");
    add(t, "Glass", 2, 9, 5, "The glass machine's cycle, in steps");
    add(t, "Wood", 2, 9, 7, "The wood machine's cycle, in steps");
    add_pick(t, "Mesh", 3, 0, SC_MESH_N, "How the machines' cycles fit together");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(TIN) = (int16_t)(2 + dice_int(8));
    P(GLASS) = (int16_t)(2 + dice_int(8));
    P(WOOD) = (int16_t)(2 + dice_int(8));
    P(MESH) = (int16_t)dice_int(3);
    F(deal)(t);
}

static void F(on_step)(toy_t *t)
{
    scrap_t *s = ST(scrap_t);
    int m, now = toy_now(t);
    for (m = 0; m < 3; m++) {
        int period = P(m), at;
        if (P(MESH) == 2)
            period = period * 3 / 2;            /* odd: everything half as long again */
        if (period < 2)
            period = 2;
        at = P(MESH) == 1 ? (now % BAR) : (now + s->off[m]);
        if (at % period == 0)
            toy_note(t, s->deg[m], m, 0);
    }
    if (dice_lt_q(4915))                        /* a loose part rattling, 7.5% of steps */
        toy_note(t, 0, 3, 0);
}

TOY(toy_scrap, "scrap", "Scrapyard", "gears", ROLE_DRUMS, HUE_GREY, 0, 2,
    "Machines for drums: clanks of tin, glass and wood, each on a cycle of its own.",
    scrap_t, F(make), 0, 0, F(randomise), F(on_step), 0, 0, 0, 0);
#undef TIN
#undef GLASS
#undef WOOD
#undef MESH
