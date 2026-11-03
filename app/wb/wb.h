/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1: the Witchbox toys of Playbox, in C for the FM-1.
 *
 * Everything a toy needs is here: its dice (dice.c), the shared settings — scale, chords, tempo,
 * what the toys tell each other (settings.c) — and the toy itself: its knobs, its clock and the
 * way it is started, restarted and shaken (toy.c). The toys are in toys/, one file each, and
 * toys/all.c lists them.
 *
 * The rule of the port: a toy here rolls its dice in the same order and does the same integer
 * sums as the Python toy it comes from, so the same seed and knobs play the same notes on the
 * Pico and on the FM-1 (ref/run_ref.py against ref/run_c.c checks that). Where the Python did
 * arithmetic on rand() (0 <= r < 1, a 16-bit fraction), the C does the same sum on the fraction's
 * 16-bit numerator with integers, which is exact — see dice.h. No double anywhere (the device
 * build refuses one); float only in drawing. */
#pragma once
#include <stdint.h>
#include "dice.h"

/* ---------------------------------------------------------------------------------- params --- */
typedef struct {
    const char *name;
    int16_t lo, hi, value, step;
    const char *const *choices;          /* names for lo..hi; a list of names wraps round */
    const char *fmt;                     /* "%d", "%d%%", "%04d" — see param_text */
    const char *help;
} param_t;

void param_init(param_t *p, const char *name, int lo, int hi, int value, int step,
                const char *const *choices, const char *fmt, const char *help);
int param_turn(param_t *p, int detents);      /* 1 = it changed */
void param_text(const param_t *p, char *b);   /* 12 bytes */
int param_frac16(const param_t *p);           /* 0..65536 for a knob's dot */

#define MAX_PARAMS 8

/* ---------------------------------------------------------------------------------- shared --- */
enum { ROLE_NONE, ROLE_DRUMS, ROLE_BASS, ROLE_CHORDS, ROLE_LEAD, ROLE_VOICE, ROLE_TEXTURE,
       ROLE_CONDUCTOR, NROLES };
extern const char *const ROLE_NAME[NROLES];

#define NSCALES 9
#define NDARK 5
extern const char *const SCALE_NAME[NSCALES];
extern const uint8_t DARK_SCALES[NDARK];       /* the set the Witchbox toys cycle through */
extern const int8_t SCALE_SEMIS[NSCALES][7];

#define NPROG 15
typedef struct { const char *name; uint8_t n; uint8_t root[12]; } progression_t;
extern const progression_t PROGRESSIONS[NPROG];
#define BAR 16                                 /* clock steps in a bar */
#define SEATS 8

typedef struct {
    param_t scale, tempo, volume, rate;        /* the strip */
    /* Chords: 0 = Random (rolled), else PROGRESSIONS[chords - 1] */
    int chords;
    uint8_t rolled[4];
    /* what toys tell each other (the band owns these; a toy alone uses the same names) */
    int band;                                  /* 1 while the band is open: its Chords rule */
    int band_steps;
    int kicks, drummer, lead, leads, has_lead;
    uint8_t roles[SEATS];
    uint8_t cuts[SEATS];
    uint8_t levels[SEATS];
    int speed, drop;                           /* Screw: clock %, pitch drop 0..10 */
    int root_hold;                             /* a key held: the chord root forced to this, else -1 */
    int tone;                                  /* Fog: 0 dull .. 50 as designed .. 100 bright */
    uint8_t throw_[SEATS];                     /* Dubout: how far each seat is thrown into the echo, 0..127 */
} settings_t;
extern settings_t S;

void settings_init(void);
void settings_calm(void);
int chord_bar(int steps);
void chords_roll(void);                        /* a new Random progression, from the dice in use */
void chords_randomise(void);
int chords_root(int steps, int ahead);         /* the chord root for clock step `steps` */
const char *chords_text(char *b);
int scale_semis(int degree);                   /* semitones above the key for a scale step */
/* a note sounded: every toy's note() ends here (sound, the kick count, the last lead note) */
void heard(int role, int degree, int voice, int module, int seat, int hold);

/* ---------------------------------------------------------------------------------- the toy --- */
typedef struct toy toy_t;

typedef struct {
    const char *module, *name, *glyph, *words;
    uint8_t role, hue, frame_ms, page;         /* page 0 = Witchbox III, 1 = IV */
    void (*make)(toy_t *t);                    /* make_params: knobs (and rolls) and state */
    int (*length)(toy_t *t);                   /* steps in its loop; 0 = 16 */
    void (*changed)(toy_t *t);                 /* after any knob changes */
    void (*randomise)(toy_t *t);               /* new values for its own knobs */
    void (*on_step)(toy_t *t);
    void (*play_changed)(toy_t *t);
    void (*frame)(toy_t *t, int ms);
    void (*draw)(toy_t *t);                    /* screen: the toy's picture (ui.c calls it) */
    void (*close)(toy_t *t);                   /* put away (a conductor lets go) */
    void (*opened)(toy_t *t);                  /* what Playbox's draw_main did to the toy's state */
    uint16_t state_size;
} toy_class_t;

#define MAX_STATE 512                          /* bytes of state the biggest toy may have (the largest is about 250) */

struct toy {
    const toy_class_t *c;
    int id;                                    /* index in TOYS */
    param_t p[MAX_PARAMS];
    int np;
    uint16_t seed;
    dice_t dice;
    int playing, pos, steps, step_ms, acc, facc;
    int seat;                                  /* which band seat sounds its notes (0 alone) */
    int kicks_seen;                            /* Dark.new_kick */
    uint32_t state[MAX_STATE / 4];
};

extern const toy_class_t *const TOYS[];
extern const int NTOYS;
int toy_find(const char *module);

void toy_open(toy_t *t, int id, int seed);     /* build it from this seed, knobs at their defaults */
void toy_load(toy_t *t, const int16_t *vals);  /* saved knob values (then changed(), twice as Playbox) */
void toy_restart(toy_t *t);                    /* Play: the dice back to the seed, rebuilt, knobs kept */
void toy_shake(toy_t *t, int seed);            /* Random: this seed and the settings that go with it */
void toy_play(toy_t *t, int on);
void toy_tick(toy_t *t, int ms);               /* ms of time, in one fixed slice */
void toy_turn(toy_t *t, int k, int detents);   /* knob k turned */
void toy_scale_next(toy_t *t);
void toy_note(toy_t *t, int degree, int voice, int hold);   /* voice -1 = None */
void toy_close(toy_t *t);
int toy_length(toy_t *t);

/* Dark helpers */
int toy_root(toy_t *t, int ahead);             /* the current chord's root, 0..6 */
int toy_now(toy_t *t);                         /* the clock step everyone agrees on */
int toy_bar(toy_t *t);
void toy_kick(toy_t *t);
int toy_new_kick(toy_t *t);

/* tune.py */
int tune_nearest(int note, int root);          /* the chord note closest to this one */
int tune_tones(int root, int lo, int hi, int8_t *out);   /* the chord's notes between lo and hi */

/* for toys: the state block, typed */
#define ST(T) ((T *)(t->state))
#define P(i) (t->p[i].value)
