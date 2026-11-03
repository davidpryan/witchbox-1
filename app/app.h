/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 app: the band (eight seats, each a Witchbox toy), what is saved, the app's entry points and
 * its half of the contract with fm1-base (base/device/unity.c on the FM-1, base/host/plat_host.c in the
 * simulator). fm1-base's build compiles app.c after the platform, with its drawing helpers in scope. */
#pragma once
#include <stdint.h>
#include "plat.h"
#include "dsp/wb_engine.h"
#include "wb/wb.h"

#define APP_NAME "WITCHBOX1"
#define APP_TAGLINE "Witch house toys for the FM-1"
#define APP_SPLASH_RGB RGB(230, 220, 244)
#define APP_PALETTE 4u                   /* the platform's own screens (crash, safe mode, update) in mono */
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz */
#define APP_OVERLOAD                     /* the render over 90 % of its time: a voice shed */
#define EN_VERSION APP_VERSION           /* fm1b_config.h: "DEV", or a release build's own */

/* the store: the project at 0xFC000 and the panel calibration at 0xDC000 (as HAUNT-1 and AMB-1) */
enum { OBJ_PROJ, OBJ_PANEL, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u}
/* backup and restore over USB (base/tools/fm1_backup.py, the site's backup page): the project */
#define APP_BACKUP_OBJS {OBJ_PROJ}
#define APP_BACKUP_MAGIC 0x31584257u     /* "WBX1" */
#define APP_BACKUP_VERSION 1u
#define APP_BACKUP_WHAT "replaces the band and every saved toy"
int app_backup_playing(void);
void app_backup_flush(void);
void app_backup_hold(int on);
void app_backup_restored(void);
#define PROJ_MAGIC 0x31584257u           /* "WBX1": another firmware's project is not ours */
#define PROJ_FORMAT 1u

#define NSLOTS 16                        /* saved toys: the sixteen white keys */
#define NSCENES 4                        /* saved bands: four black keys */

typedef struct {                         /* a toy as it is now: enough to bring it back */
    int8_t toy;                          /* index in TOYS, -1 = empty */
    uint8_t mute;
    uint16_t seed;
    int16_t knob[4];
    uint8_t level;                       /* the seat's volume 0..100 */
    uint8_t rsv;
} seat_rec_t;

/* how the seats are arranged: everyone always (Band), a lane of clips each (Tape), or a ring of
 * sixteen dots each (Clock); the seat sounds only in the bars its arrangement gives it */
enum { MODE_BAND, MODE_TAPE, MODE_CLOCK, NMODES };
#define TAPE_MAX 64                      /* bars in a lane */

typedef struct {                         /* a whole band */
    seat_rec_t seat[SEATS];
    uint8_t scale, chords, rate, tempo;  /* tempo: (bpm - 40) / 2 */
    uint8_t rolled[4];
    uint8_t fx[4];                       /* drive, crush, echo, feedback 0..127 */
    uint8_t mode, tape_len;              /* MODE_*; the tape's bars, 4..64 */
    uint8_t tape[SEATS][TAPE_MAX / 8];   /* each lane: a bit per bar */
    uint8_t dots[SEATS], rot[SEATS], span[SEATS];   /* each ring: dots of 16, rotation, bars a dot lasts (0: 1, 1: 2, 2: 4) */
} scene_t;

typedef struct {
    uint32_t magic, format;
    scene_t now;                         /* the band as it is */
    uint8_t cur;                         /* the seat on screen */
    uint8_t leds;                        /* lights: 0 off, 1 keys and the buttons' glow, 2 keys only */
    uint8_t volume;                      /* overall level 0..100 */
    uint8_t rsv0;
    seat_rec_t slot[NSLOTS];             /* saved toys */
    scene_t scene[NSCENES];              /* saved bands */
    uint8_t rsv[16];
} project_t;

extern project_t proj;

void project_defaults(void);
int project_load(void);                  /* 0 = loaded; else defaults are in place */
int project_save(void);                  /* 0 = saved and verified */
void project_apply(void);                /* the band from proj.now (boot, load) */
void project_keep(void);                 /* the band into proj.now */

/* the band (band.c) */
extern toy_t band[SEATS];
extern uint8_t band_playing;
extern int band_solo;
void band_init(void);
void band_seat(int i, int toy, int seed);        /* a toy into seat i (-1 empties it) */
void band_play(int on);
void band_tick(uint32_t now_ms);                 /* run the clock from the main loop */
void band_shake(int i);                          /* Random: a new seed for seat i */
void band_seed(int i, int seed);                 /* the seed set by hand */
int band_undo_seed(int i);                       /* the seed before the last Random; returns it */
void band_turn(int i, int k, int d);             /* seat i's knob k */
void band_fill(uint32_t entropy);                /* a random band: drums, bass, chords, lead */
int band_seated(int i);
int band_bar_on(int i, int bar);                 /* does seat i sound in this bar of the arrangement? */
int band_bars(void);                             /* bars before the arrangement comes round */
void band_fill_arrangement(uint32_t entropy);    /* lay the seats out at random on the tape or the clock */
uint32_t euclid16(int k, int rot);               /* k of 16 dots, spread evenly, turned by rot (bit 0 = dot 0) */

/* what the toys sounded lately, for the screen and the lights */
typedef struct { uint8_t seat, voice, role; int8_t degree; uint32_t at; } note_log_t;
#define NOTE_LOG 32
extern note_log_t note_log[NOTE_LOG];
extern uint32_t note_log_n;

void sound_init(void);
void sound_play(int role, int degree, int voice, int module, int seat, int hold);
void sound_hush(void);
void sound_resend(void);
extern int seat_level[SEATS];

void ui_init(void);
void ui_frame(void);
void ui_input_only(void);
int ui_dirty(void);
void ui_say(const char *a, const char *b);
int ui_held(void);                       /* 1 while a backup asks the UI not to save */
void ui_hold(int on);

/* the contract: the platform calls these */
void app_init(void);                     /* boot: the engine, the band, the saved project (or the defaults) */
void app_render(int32_t *lr, uint32_t n, uint32_t gain_q12);   /* audio interrupt: n stereo frames, 24-bit */
void app_panic(void);                    /* silence now (before an update rewrites the flash) */
void app_overload(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

/* the host simulator's part (app/host.c): witchbox1's checks and commands */
#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
#define APP_HOST_CMD
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
#define APP_HOST_REBOOT
void app_host_reboot(void);
