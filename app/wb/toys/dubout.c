/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 100 (Witchbox III, conductor): Dub Out. At the end of each phrase, throws one part of the
 * band into the echo for a beat or a bar, then lets it back. S.throw_ carries it to the sound. */
#include "toy_util.h"
#undef F
#define F(n) du_##n

static const uint8_t DU_PHRASE[3] = {32, 64, 128};
static const uint8_t DU_LEN[3] = {4, 8, 16};
static const uint8_t DU_WHO[4] = {ROLE_NONE, ROLE_DRUMS, ROLE_LEAD, ROLE_CHORDS};
static const char *const DU_PHRASE_N[3] = {"2 bars", "4 bars", "8 bars"};
static const char *const DU_LEN_N[3] = {"1 beat", "2 beats", "1 bar"};
static const char *const DU_WHO_N[4] = {"Any part", "Drums", "Lead", "Chords"};
typedef struct { int thrown; } dubout_t;
#define THROW 0
#define PHRASE 1
#define LENGTH 2
#define WHO 3

static void F(make)(toy_t *t)
{
    add(t, "Throw", 0, 10, 7, "How much of the phrase's end is thrown");
    add_pick(t, "Phrase", 3, 1, DU_PHRASE_N, "How long each phrase is");
    add_pick(t, "Length", 3, 1, DU_LEN_N, "How long the part stays in the echo");
    add_pick(t, "Who", 4, 0, DU_WHO_N, "Which part gets thrown");
    ST(dubout_t)->thrown = -1;
}

static void F(randomise)(toy_t *t)
{
    P(THROW) = (int16_t)(3 + dice_int(8));
    P(PHRASE) = (int16_t)dice_int(3);
    P(LENGTH) = (int16_t)dice_int(3);
    P(WHO) = (int16_t)dice_int(4);
}

static void F(let_go)(toy_t *t)
{
    int i;
    for (i = 0; i < SEATS; i++)
        S.throw_[i] = 0;
    ST(dubout_t)->thrown = -1;
}

static void F(close)(toy_t *t) { F(let_go)(t); }
static void F(play_changed)(toy_t *t) { F(let_go)(t); }

static int F(pick)(toy_t *t)                    /* a seat to throw: the wanted role, else any player */
{
    int i, n = 0, want = DU_WHO[P(WHO)];
    uint8_t cand[SEATS];
    if (!S.band)
        return 0;
    for (i = 0; i < SEATS; i++)
        if (i != t->seat && S.roles[i] != ROLE_NONE && S.roles[i] != ROLE_CONDUCTOR &&
            (want == ROLE_NONE || S.roles[i] == want))
            cand[n++] = (uint8_t)i;
    if (n == 0)
        for (i = 0; i < SEATS; i++)
            if (i != t->seat && S.roles[i] != ROLE_NONE && S.roles[i] != ROLE_CONDUCTOR)
                cand[n++] = (uint8_t)i;
    return n ? cand[dice_int(n)] : -1;
}

static void F(on_step)(toy_t *t)
{
    dubout_t *s = ST(dubout_t);
    int period = DU_PHRASE[P(PHRASE)], len = DU_LEN[P(LENGTH)], at = toy_now(t) % period;
    if (at == period - len && s->thrown < 0 && P(THROW) > 0) {
        int seat = F(pick)(t);
        if (seat >= 0) {
            s->thrown = seat;
            S.throw_[seat] = (uint8_t)(P(THROW) * 12);
        }
    } else if (at == 0 && s->thrown >= 0) {
        F(let_go)(t);
    }
}

#ifdef WB_PICTURES
/* the picture: the phrase as a bar that fills; at its end the thrown part's seat is named and rings
 * spread from it into the echo */
static void F(draw)(toy_t *t)
{
    dubout_t *s = ST(dubout_t);
    int period = DU_PHRASE[P(PHRASE)], at = toy_now(t) % period, i;
    char b[12] = "SEAT ";
    cv_rect(20, 60, 200, 10, K_PANEL);
    cv_rect(20, 60, at * 200 / period, 10, K_TEXT);
    cv_rect(20 + (period - DU_LEN[P(LENGTH)]) * 200 / period, 56, 2, 18, pic_hue);
    if (s->thrown >= 0) {
        b[5] = (char)('1' + s->thrown);
        b[6] = 0;
        cv_text(96, 100, &FONT_XS, b, pic_hue);
        for (i = 0; i < 3; i++)
            pic_ring(120, 90, 16 + i * 10 + (int)((plat_ms() / 50u) % 10u), 15 + i * 10 + (int)((plat_ms() / 50u) % 10u), i == 0 ? pic_hue : K_SOFT);
    } else
        cv_text(84, 100, &FONT_XS, "NEXT THROW", K_DIM);
}
#endif

TOY(toy_dubout, "dubout", "Dub Out", "echo", ROLE_CONDUCTOR, HUE_GO, 0, 2, "A conductor. Throws one part into the echo at the end of each phrase.", dubout_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), F(close));
#undef THROW
#undef PHRASE
#undef LENGTH
#undef WHO
