/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 sound engine: Playbox's pbaudio (engine.cpp) written out in plain C for the FM-1.
 *
 * Same shape as Playbox's: a few small pools of voices, one pool per kind of engine (analog, fm,
 * string, organ, voice, boom, sample, noise); a note picks a free voice in the right pool or takes
 * the oldest. Each instrument is a patch: an engine and its settings. Drums are single instances
 * struck again. It renders at 22,050 Hz, as Playbox did (and as the recordings are), and the
 * output is doubled up to the FM-1's 44,100 Hz with a little smoothing.
 *
 * Threads: en_render() runs in the audio interrupt. Everything else posts commands into a ring
 * that the render drains first. */
#pragma once
#include <stdint.h>

#define EN_SR 22050
#define EN_OUT_SR 44100
#define EN_SEATS 8

/* instruments (Playbox's sound_ids: the same order) */
enum {
    I_SUB, I_808_BOOM, I_808_SLIDE, I_ACID, I_REESE, I_SAW_BASS, I_FM_BASS, I_PLUCK_BASS, I_SOFT_KEYS,
    I_ELECTRIC_PIANO, I_DULL_PIANO, I_TOY_PIANO, I_CLAV, I_MUSIC_BOX, I_SOUL_ORGAN, I_CHURCH_ORGAN,
    I_CHOIR_AAH, I_CHOIR_OOH, I_WARM_PAD, I_DARK_STRINGS, I_GLASS_PAD, I_SAW_PAD, I_COVEN_SAWS, I_DRONE,
    I_SAW_LEAD, I_SQUARE_LEAD, I_CHIP_LEAD, I_GLIDE_LEAD, I_SCREAMER, I_WHISTLE, I_GLASS_BELL, I_KALIMBA,
    I_MARIMBA, I_HARP, I_GUITAR, I_TRAP_BELL, I_RAVE_STAB, I_MINOR_STAB, I_GHOST_VOICE, I_SCREWED_VOICE,
    I_CRACKLE, I_RAIN, I_RUMBLE, I_CITY_HUM, I_RISER, I_IMPACT, I_WOOSH, I_DOOR, I_CROW, I_SKIP, I_VOX_HIT,
    I_TOLL_BELL, I_COLD_KEYS, I_THUNDER,     /* witchbox1's own, for Witch III */
    I_COUNT
};
/* drums */
enum {
    D_KICK, D_LONG_KICK, D_HARD_KICK, D_SNARE, D_RIM, D_CLAP, D_LONG_CLAP, D_CLOSED_HAT, D_OPEN_HAT,
    D_PEDAL_HAT, D_CRASH, D_RIDE, D_LOW_TOM, D_MID_TOM, D_HIGH_TOM, D_SHAKER, D_COWBELL, D_CONGA, D_CLICK,
    D_FOUND, D_COUNT
};
enum { K_808, K_909, K_ACOUSTIC, K_LO_FI, K_WITCH, K_GARAGE, K_CHIP, K_TIN, K_COUNT };
#define DRUM_BASE 64

enum {
    EN_NOTE = 1,        /* seat, sound, note (MIDI number), vel 0..127, arg = hold ms (0 = the sound's own)
                           for drums: sound = DRUM_BASE + drum, note = semitones up or down, arg = kit
                           for "Found" and "Click": note = which one */
    EN_SEAT_LEVEL,      /* seat, vel = level 0..127 */
    EN_SEAT_TONE,       /* seat, vel = brightness 0..127 (64 = as designed) */
    EN_MASTER,          /* vel = level 0..127 */
    EN_DROP,            /* note = how far everything is pitched down, in sixteenths of a semitone */
    EN_ECHO,            /* vel = how much echo 0..127, arg = echo time in ms */
    EN_FX,              /* note = which (0 drive, 1 crush, 2 delay time ms in arg, 3 feedback), vel = amount */
    EN_ALL_OFF,         /* let every held note go */
    EN_PANIC,           /* everything quiet, now (en_panic posts this: the render does it) */
    EN_SEAT_SEND,       /* seat, vel = extra echo send 0..127 (a conductor throwing a part into the echo) */
};

typedef struct {
    uint8_t op, seat, sound, vel;
    int16_t note;
    uint16_t arg;
} en_cmd_t;

void en_init(void);
void en_post(const en_cmd_t *c);                   /* from the main loop */
void en_note(int seat, int sound, int note, int vel, int arg);
void en_set(int op, int seat, int vel, int note, int arg);
void en_render(int32_t *out_lr, uint32_t n, uint32_t gain_q12);   /* 44.1 kHz stereo, the audio ISR */
void en_panic(void);                                /* everything quiet, now (posted; safe from the main loop) */
void en_play(int on);                               /* (kept for the platform: nothing to start here) */
int en_active(void);                                /* voices sounding now */
int en_shed(void);                                  /* drop the oldest note */
void en_midi_out(uint32_t st, uint32_t d1, uint32_t d2);   /* the platform's */
int en_sound_by_name(const char *name);
const char *en_sound_name(int i);
