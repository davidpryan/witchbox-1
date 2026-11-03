/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 119 (Witchbox IV, conductor): Culling. Every phrase, draws lots: each seat has a chance of
 * being cut for the whole phrase, so the band thins and fills at random, phrase by phrase. The
 * drums can be spared. */
#include "toy_util.h"
#undef F
#define F(n) cu_##n

static const uint8_t CU_EVERY[3] = {32, 64, 128};
static const char *const CU_EVERY_N[3] = {"2 bars", "4 bars", "8 bars"};
static const char *const CU_SPARE_N[3] = {"No one", "The drums", "Drums and bass"};
typedef struct { int armed; } culling_t;
#define CHANCE 0
#define EVERY 1
#define SPARE 2
#define ATLEAST 3

static void F(let_go)(void)
{
    int i;
    for (i = 0; i < SEATS; i++)
        S.cuts[i] = 0;
}

static void F(make)(toy_t *t)
{
    add(t, "Chance", 0, 10, 4, "Each seat's chance of being cut for a phrase");
    add_pick(t, "Every", 3, 1, CU_EVERY_N, "How long a phrase is");
    add_pick(t, "Spare", 3, 1, CU_SPARE_N, "Who is never cut");
    add(t, "At least", 1, 4, 2, "How many seats always stay");
    ST(culling_t)->armed = 1;
}

static void F(randomise)(toy_t *t)
{
    P(CHANCE) = (int16_t)(2 + dice_int(8));
    P(EVERY) = (int16_t)dice_int(3);
    P(SPARE) = (int16_t)dice_int(3);
    P(ATLEAST) = (int16_t)(1 + dice_int(4));
}

static void F(close)(toy_t *t) { (void)t; F(let_go)(); }
static void F(play_changed)(toy_t *t) { ST(culling_t)->armed = 1; F(let_go)(); }

static void F(on_step)(toy_t *t)
{
    int i, kept = 0, period = CU_EVERY[P(EVERY)];
    if (!S.band || toy_now(t) % period != 0)
        return;
    for (i = 0; i < SEATS; i++) {
        int role = S.roles[i], spared = i == t->seat || role == ROLE_NONE || role == ROLE_CONDUCTOR ||
                                       (P(SPARE) >= 1 && role == ROLE_DRUMS) || (P(SPARE) >= 2 && role == ROLE_BASS);
        S.cuts[i] = (uint8_t)(!spared && dice_lt(P(CHANCE), 11));
        if (role != ROLE_NONE && role != ROLE_CONDUCTOR && !S.cuts[i])
            kept++;
    }
    for (i = 0; i < SEATS && kept < P(ATLEAST); i++)   /* not everyone: let some back in */
        if (S.cuts[i]) {
            S.cuts[i] = 0;
            kept++;
        }
}

#ifdef WB_PICTURES
/* the picture: a bar for each seat, white while it plays and dark for the phrase it is cut; the phrase
 * as a line that runs out to the next draw */
static void F(draw)(toy_t *t)
{
    int i, n = S.band ? SEATS : 4, period = CU_EVERY[P(EVERY)], at = toy_now(t) % period;
    for (i = 0; i < n; i++) {
        int x = pic_col(i, n, 9), w = n > 4 ? 20 : 33, role = S.band ? S.roles[i] : ROLE_NONE;
        uint16_t c = S.cuts[i] ? K_PANEL : role != ROLE_NONE || !S.band ? K_TEXT : K_SAND;
        cv_rect(x - w / 2, 16, w, 90, c);
        if (S.cuts[i])
            cv_line(x - w / 2, 16, x + w / 2, 106, pic_hue), cv_line(x + w / 2, 16, x - w / 2, 106, pic_hue);
    }
    cv_rect(12, 124, 216, 3, K_PANEL);
    cv_rect(12, 124, (period - at) * 216 / period, 3, pic_hue);
    (void)t;
}
#endif

TOY(toy_culling, "culling", "Culling", "lots", ROLE_CONDUCTOR, HUE_RED, 0, 3, "A conductor. Every phrase draws lots: each seat may be cut for the phrase, so the band thins and fills at random.", culling_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), F(close));
#undef CHANCE
#undef EVERY
#undef SPARE
#undef ATLEAST
