/* SPDX-License-Identifier: GPL-3.0-only */
/* Turns what a toy plays into what the sound engine is asked for (Playbox's sound.py).
 *
 * A toy only ever says note(step of the scale, voice). This decides which instrument or drum that
 * is, which actual note, and how loud, and posts it to the engine. Every Witchbox toy has its own
 * sounds, listed in WITCH below: either one instrument for every voice, or a kit and one entry per
 * voice number (a drum, an instrument played at the note asked for, a found sound, a click, or
 * several of these at once). */
#include "wb.h"
#include "../../dsp/wb_engine.h"

#define KEY 60                      /* scale step 0 is middle C */

enum { V_NONE, V_DRUM, V_INST, V_FOUND, V_CLICK, V_INST2 };     /* V_INST2: two instruments at once */
typedef struct { uint8_t kind, a, b, vel; } voice_t;   /* vel 0 = full (100) */
#define DR(d) {V_DRUM, D_##d, 0, 0}
#define DRV(d, v) {V_DRUM, D_##d, 0, v}
#define IN(i) {V_INST, I_##i, 0, 0}
#define INV(i, v) {V_INST, I_##i, 0, v}
#define IN2(i, j) {V_INST2, I_##i, I_##j, 0}
#define FD(n) {V_FOUND, n, 0, 0}
#define CK(n) {V_CLICK, n, 0, 0}
#define NO {V_NONE, 0, 0, 0}

typedef struct {
    const char *module;
    int8_t inst;                    /* one instrument (I_*) for every voice, or -1 for a kit */
    int8_t octaves;
    uint8_t kit;
    voice_t v[7];
} witch_t;

static const witch_t WITCH[] = {
    {"twostep", -1, 0, K_GARAGE, {DR(KICK), DR(RIM), DR(CLOSED_HAT), CK(2), NO, NO, NO}},
    {"hatroll", -1, 0, K_WITCH, {NO, NO, DR(CLOSED_HAT), DR(OPEN_HAT), NO, NO, NO}},
    {"stomp", -1, 0, K_WITCH, {DR(LONG_KICK), DR(LONG_CLAP), NO, NO, NO, NO, NO}},
    {"pocket", -1, 0, K_LO_FI, {NO, NO, NO, FD(0), FD(2), FD(3), FD(4)}},
    {"limp", -1, 0, K_LO_FI, {DR(KICK), DR(SNARE), NO, NO, NO, NO, NO}},
    {"fourfloor", -1, 0, K_909, {DR(HARD_KICK), DR(CLAP), DR(CLOSED_HAT), DR(OPEN_HAT), NO, NO, NO}},
    {"rush", -1, 0, K_909, {NO, DR(SNARE), NO, DR(CRASH), NO, NO, NO}},
    {"clock", -1, 0, K_LO_FI, {DR(KICK), CK(3), CK(1), NO, NO, NO, NO}},
    {"toms", -1, 0, K_WITCH, {NO, NO, NO, DR(LOW_TOM), DR(MID_TOM), DR(HIGH_TOM), NO}},
    {"riser", -1, 0, K_WITCH, {IN(RISER), IN(GLIDE_LEAD), IN(IMPACT), IN(WOOSH), NO, NO, NO}},
    {"crackle", -1, 0, K_LO_FI, {CK(2), IN2(RAIN, CRACKLE), IN(RUMBLE), IN(SKIP), NO, NO, NO}},
    {"nightbus", -1, 0, K_LO_FI, {IN(CITY_HUM), IN(GLIDE_LEAD), IN(WOOSH), DR(SHAKER), IN(DOOR), NO, NO}},
    {"blackout", -1, 0, K_WITCH, {IN(RISER), NO, NO, NO, NO, NO, NO}},
    {"undertow", I_SUB, -1, 0, {NO}}, {"slide808", I_808_SLIDE, -1, 0, {NO}}, {"octpump", I_SAW_BASS, 0, 0, {NO}},
    {"reese", I_REESE, 0, 0, {NO}}, {"drone", I_DRONE, 1, 0, {NO}},
    {"pumppad", I_WARM_PAD, 0, 0, {NO}}, {"coven", I_COVEN_SAWS, 0, 0, {NO}}, {"gate", I_SAW_PAD, 0, 0, {NO}},
    {"wetpiano", I_DULL_PIANO, 0, 0, {NO}}, {"cathedral", I_CHURCH_ORGAN, 0, 0, {NO}},
    {"glassarp", I_GLASS_BELL, 0, 0, {NO}}, {"bitarp", I_CHIP_LEAD, 0, 0, {NO}}, {"hook", I_SAW_LEAD, 0, 0, {NO}},
    {"lost", I_WHISTLE, 0, 0, {NO}}, {"screamer", I_SCREAMER, 0, 0, {NO}},
    {"rave", I_RAVE_STAB, 0, 0, {NO}}, {"swell", I_WARM_PAD, 0, 0, {NO}}, {"tapeloop", I_SOFT_KEYS, 0, 0, {NO}},
    {"musicbox", I_MUSIC_BOX, -1, 0, {NO}}, {"siren", I_GLIDE_LEAD, 0, 0, {NO}}, {"shadow", I_SOFT_KEYS, 0, 0, {NO}},
    {"ghost", I_GHOST_VOICE, 0, 0, {NO}}, {"chop", I_VOX_HIT, 0, 0, {NO}}, {"screwed", I_SCREWED_VOICE, 0, 0, {NO}},
    {"breath", I_CHOIR_OOH, 0, 0, {NO}},
    /* Witch III */
    {"heart", -1, 0, K_808, {DR(KICK), DRV(KICK, 55), NO, NO, NO, NO, NO}},
    {"chopbreak", -1, 0, K_ACOUSTIC, {DR(KICK), DR(SNARE), DR(CLOSED_HAT), DR(OPEN_HAT), DR(CRASH), NO, NO}},
    {"scrap", -1, 0, K_TIN, {FD(0), FD(2), FD(3), CK(2), NO, NO, NO}},
    {"rattle", I_MARIMBA, 0, 0, {NO}},
    {"phonk", -1, 0, K_808, {DR(HARD_KICK), DR(CLAP), DR(COWBELL), DRV(CLOSED_HAT, 70), NO, NO, NO}},
    {"bodybass", I_PLUCK_BASS, -1, 0, {NO}}, {"coldwave", I_COLD_KEYS, -1, 0, {NO}}, {"drip", I_ACID, -1, 0, {NO}},
    {"afterhours", I_ELECTRIC_PIANO, 0, 0, {NO}},
    {"vespers", -1, 0, 0, {IN(CHOIR_AAH), INV(CHOIR_OOH, 70), NO, NO, NO, NO, NO}},
    {"graveyard", I_GUITAR, 0, 0, {NO}}, {"shiver", I_DARK_STRINGS, 0, 0, {NO}},
    {"hexbell", I_GLASS_BELL, 0, 0, {NO}}, {"cartridge", I_SQUARE_LEAD, 0, 0, {NO}}, {"helium", I_GHOST_VOICE, 1, 0, {NO}},
    {"storm", -1, 0, 0, {IN(RAIN), IN(RUMBLE), IN(THUNDER), IN(WOOSH), NO, NO, NO}},
    {"murder", -1, 0, 0, {IN(CROW), INV(CROW, 45), INV(WOOSH, 70), NO, NO, NO, NO}},
    {"toll", I_TOLL_BELL, -1, 0, {NO}},
    /* Witch IV */
    {"turing", -1, 0, K_909, {DR(KICK), DR(SNARE), DR(CLOSED_HAT), DR(RIM), NO, NO, NO}},
    {"ratchet", -1, 0, K_ACOUSTIC, {DR(KICK), DR(SNARE), DR(CLOSED_HAT), DRV(SNARE, 50), NO, NO, NO}},
    {"markovkit", -1, 0, K_WITCH, {DR(KICK), DR(SNARE), DR(CLOSED_HAT), DR(OPEN_HAT), DR(RIM), DR(LOW_TOM), NO}},
    {"automaton", -1, 0, K_CHIP, {DR(KICK), DR(SNARE), DR(CLOSED_HAT), NO, NO, NO, NO}},
    {"sleepwalk", I_SUB, 0, 0, {NO}}, {"pulsar", I_SAW_BASS, -1, 0, {NO}}, {"hysteria", I_FM_BASS, -1, 0, {NO}},
    {"seance", I_GLASS_PAD, 0, 0, {NO}}, {"lichen", I_SOUL_ORGAN, 0, 0, {NO}}, {"round", I_HARP, 0, 0, {NO}},
    {"fractal", I_TOY_PIANO, 0, 0, {NO}}, {"oracle", I_CLAV, 0, 0, {NO}}, {"loom", I_KALIMBA, 0, 0, {NO}},
    {"golden", I_MUSIC_BOX, 0, 0, {NO}}, {"haunting", I_WHISTLE, 0, 0, {NO}},
    {"chimes", I_GLASS_BELL, 1, 0, {NO}},
    {"whisper", -1, 0, 0, {INV(CHOIR_OOH, 55), INV(CHOIR_AAH, 45), NO, NO, NO, NO, NO}},
    {"static", -1, 0, K_LO_FI, {IN(CRACKLE), IN(RUMBLE), IN(SKIP), CK(1), NO, NO, NO}},
};
#define NWITCH ((int)(sizeof WITCH / sizeof WITCH[0]))
static int8_t witch_of[128];         /* toy id -> row of WITCH, or -1 (conductors: no sound) */

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b)
        a++, b++;
    return *a == 0 && *b == 0;
}

void sound_init(void)
{
    int i, j;
    for (i = 0; i < NTOYS && i < 128; i++) {
        witch_of[i] = -1;
        for (j = 0; j < NWITCH; j++)
            if (streq(TOYS[i]->module, WITCH[j].module))
                witch_of[i] = (int8_t)j;
    }
}

static int fit(int note)           /* fold a note into the range the instruments sound good in */
{
    while (note > 96)
        note -= 12;
    while (note < 24)
        note += 12;
    return note;
}

int seat_level[SEATS] = {80, 80, 80, 80, 80, 80, 80, 80};   /* the band's volume for each seat, 0..100 */
static int sent_master = -1, sent_level[SEATS] = {-1, -1, -1, -1, -1, -1, -1, -1}, sent_drop = -1, sent_echo = -1;
static int sent_tone = -1, sent_throw[SEATS] = {-1, -1, -1, -1, -1, -1, -1, -1};

static void settings_out(int seat)  /* pass on anything that changed */
{
    int m = S.volume.value * 127 / 100, l = (S.band ? seat_level[seat] : 80) * 127 / 100;
    int d = S.drop * 8, e = 45000 / S.tempo.value;
    if (e > 400)
        e = 400;
    if (m != sent_master) {
        sent_master = m;
        en_set(EN_MASTER, 0, m, 0, 0);
    }
    if (l != sent_level[seat]) {
        sent_level[seat] = l;
        en_set(EN_SEAT_LEVEL, seat, l, 0, 0);
    }
    if (d != sent_drop) {
        sent_drop = d;
        en_set(EN_DROP, 0, 0, d, 0);
    }
    if (e != sent_echo) {
        sent_echo = e;
        en_set(EN_ECHO, 0, 64, 0, e);      /* three steps long */
    }
    if (S.tone != sent_tone) {              /* Fog: every seat duller or brighter */
        int i, v = 16 + S.tone * 96 / 100;
        sent_tone = S.tone;
        for (i = 0; i < SEATS; i++)
            en_set(EN_SEAT_TONE, i, v, 0, 0);
    }
    if (S.throw_[seat] != sent_throw[seat]) {
        sent_throw[seat] = S.throw_[seat];
        en_set(EN_SEAT_SEND, seat, S.throw_[seat], 0, 0);
    }
}

void sound_resend(void)            /* after the engine was reset */
{
    int i;
    sent_master = sent_drop = sent_echo = sent_tone = -1;
    for (i = 0; i < SEATS; i++)
        sent_level[i] = sent_throw[i] = -1;
}

static void one(int seat, const voice_t *v, int kit, int degree, int vel, int hold)
{
    int tune;
    if (v->vel)
        vel = v->vel;
    switch (v->kind) {
    case V_DRUM:
        tune = degree ? scale_semis(degree) : 0;
        tune = tune < -24 ? -24 : tune > 24 ? 24 : tune;
        en_note(seat, DRUM_BASE + v->a, tune, vel, kit);
        break;
    case V_INST:
        en_note(seat, v->a, fit(KEY + scale_semis(degree)), vel, hold);
        break;
    case V_INST2:
        en_note(seat, v->a, fit(KEY + scale_semis(degree)), vel, hold);
        en_note(seat, v->b, fit(KEY + scale_semis(degree)), vel, hold);
        break;
    case V_FOUND:                   /* found sounds come in pairs; take either */
        en_note(seat, DRUM_BASE + D_FOUND, v->a + ((v->a == 0 || v->a == 4) ? (degree & 1) : 0), vel, kit);
        break;
    case V_CLICK:
        en_note(seat, DRUM_BASE + D_CLICK, v->a, vel, kit);
        break;
    default:
        break;
    }
}

/* a toy sounded a note */
void sound_play(int role, int degree, int voice, int module, int seat, int hold)
{
    const witch_t *w;
    int v = voice < 0 ? 0 : voice, vel = 100;
    (void)role;
    if (module < 0 || module >= 128 || witch_of[module] < 0)
        return;
    settings_out(seat);
    w = &WITCH[witch_of[module]];
    if (w->inst >= 0) {             /* one instrument */
        if (streq(w->module, "glassarp") && v == 1)
            vel = 55;               /* the echo is quieter */
        else if (v == 1)
            vel = 60;               /* Witch III: voice 1 is quiet, voice 2 an accent */
        else if (v == 2)
            vel = 127;
        en_note(seat, w->inst, fit(KEY + 12 * w->octaves + scale_semis(degree)), vel, hold);
    } else if (v < 7) {
        one(seat, &w->v[v], w->kit, degree, vel, hold);
    }
}

void sound_hush(void) { en_set(EN_ALL_OFF, 0, 0, 0, 0); }
