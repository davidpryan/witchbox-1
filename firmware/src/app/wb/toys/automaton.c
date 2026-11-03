/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 104 (Witchbox IV, drums): Automaton. A row of sixteen cells is the bar; each bar the next
 * row is grown from it by one of Wolfram's rules (30, 90, 110, 150). Live cells are hits. */
#include "toy_util.h"
#undef F
#define F(n) am_##n

static const uint8_t AM_RULE[4] = {30, 90, 110, 150};
static const char *const AM_RULE_N[4] = {"Rule 30", "Rule 90", "Rule 110", "Rule 150"};
typedef struct { uint16_t row, start; int gen; } automaton_t;
#define RULE 0
#define SEEDS 1
#define VOICES 2
#define RESET 3

static void F(deal)(toy_t *t)
{
    automaton_t *s = ST(automaton_t);
    int i;
    s->start = 0;
    for (i = 0; i < 16; i++)
        if (dice_lt(P(SEEDS), 16))
            s->start |= (uint16_t)(1u << i);
    if (!s->start)
        s->start = 1u << 8;
    s->row = s->start;
    s->gen = 0;
}

static void F(make)(toy_t *t)
{
    add_pick(t, "Rule", 4, 0, AM_RULE_N, "Which rule grows the next bar");
    add(t, "Seeds", 1, 8, 2, "How many cells the first bar starts with");
    add(t, "Voices", 1, 3, 2, "How many drums the cells play");
    add(t, "Reset", 0, 10, 4, "How many bars before it starts again (0: never)");
    F(deal)(t);
}

static void F(randomise)(toy_t *t)
{
    P(RULE) = (int16_t)dice_int(4);
    P(SEEDS) = (int16_t)(1 + dice_int(8));
    P(VOICES) = (int16_t)(1 + dice_int(3));
    P(RESET) = (int16_t)dice_int(11);
    F(deal)(t);
}

static void F(changed)(toy_t *t) { F(deal)(t); }

static void F(grow)(toy_t *t)
{
    automaton_t *s = ST(automaton_t);
    unsigned rule = AM_RULE[P(RULE)], next = 0, i;
    for (i = 0; i < 16; i++) {
        unsigned l = (s->row >> ((i + 15) % 16)) & 1u, c = (s->row >> i) & 1u, r = (s->row >> ((i + 1) % 16)) & 1u;
        if ((rule >> ((l << 2) | (c << 1) | r)) & 1u)
            next |= 1u << i;
    }
    s->row = (uint16_t)next;
    if (++s->gen >= P(RESET) * 2 && P(RESET) > 0) {
        s->row = s->start;
        s->gen = 0;
    }
    if (!s->row)
        s->row = s->start;
}

static void F(on_step)(toy_t *t)
{
    automaton_t *s = ST(automaton_t);
    int i = t->steps % BAR, v = P(VOICES);
    if (i == 0 && t->steps > 0)
        F(grow)(t);
    if (!((s->row >> i) & 1u))
        return;
    /* which drum: the kick on the beat, the hat off it, the snare on the backbeat */
    if (i % 4 == 0 || v == 1)
        toy_kick(t);
    else if ((i == 4 || i == 12 || i % 4 == 2) && v == 3)
        toy_note(t, 0, 1, 0);
    else
        toy_note(t, 0, 2, 0);
}

TOY(toy_automaton, "automaton", "Automaton", "cells", ROLE_DRUMS, HUE_GO, 0, 3,
    "A bar of sixteen cells grows the next bar by one of Wolfram's rules. Live cells are hits.",
    automaton_t, F(make), 0, F(changed), F(randomise), F(on_step), 0, 0, 0, 0);
#undef RULE
#undef SEEDS
#undef VOICES
#undef RESET
