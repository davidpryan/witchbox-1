/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 UI: the panel and the screen. Part of the unity build (after gfx.c, band.c and
 * project.c); the host simulator includes it too.
 *
 *   KNOB 1-4      the toy's four knobs; with a button held, that layer's four:
 *                 GLO scale / chords / tempo / rate    EDIT the four digits of the seed
 *                 FX drive / crush / echo / feedback   ENV the mixer: seats 1-4 or 5-8
 *                 LFO drift on the four knobs          HOME lights / volume
 *   ALGORITHM     the toy in this seat                 PRESETS   the seed (spin for tens and hundreds)
 *   SELECT        the pages: Toy, Band, Tape, Clock, Saves
 *   PLAY          play / stop the band               SEL       the next dark scale
 *   OCT- / OCT+   the seat on screen (1-8)           REC       Random (new seed); held: a random band,
 *                                                             or on the Tape / Clock page a random arrangement
 *   HOME          the shelf: keys pick a toy, OCT± the page (Witch I, II, III)   ARP  mute this seat
 *   SEQ           the arrangement: Band, Tape, Clock (and its page)
 *   SAVE          held: a white key saves the toy there, a black key the band;
 *                 on the Saves page the keys load them       GLO tapped: help
 *   white keys    1-7 hold to steer the chord root; 8 all in; 9-16 mute seats 1-8 (with ENV held: solo);
 *                 on the Tape page, 1-16 toggle this lane's bars
 *   black keys    1 dice  2 undo  3 half time  4 double  5 freeze  6 brake  7 filter  8 crush
 *                 9 drive  10 echo throw  11 calm */
#include "../dsp/fastmath.h"

enum { PG_TOY, PG_BAND, PG_TAPE, PG_CLOCK, PG_SAVES, NPAGES };
static const char *const PAGE_NAME[NPAGES] = {"Toy", "Band", "Tape", "Clock", "Saves"};
static const char *const MODE_NAME[NMODES] = {"BAND", "TAPE", "CLOCK"};
#define NSHELF 4
static const char *const SHELF_NAME[NSHELF] = {"Witch I", "Witch II", "Witch III", "Witch IV"};
enum { L_NONE = -1 };
/* the mixer's four knobs: seats 1-4 while a seat of those is on screen, 5-8 otherwise */
static int mix_seat(int k) { return (proj.cur / 4) * 4 + k; }

static const uint8_t WHITE[16] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
static const uint8_t BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static int8_t key_white[27], key_black[27];     /* key -> white 0..15 / black 0..10 or -1 */

/* the screen's colours: a dark room, cream text, one colour per hue */
#define K_BG RGB(20, 20, 26)
#define K_PANEL RGB(40, 40, 50)
#define K_LINE RGB(66, 66, 80)
#define K_DIM RGB(140, 136, 150)
#define K_TEXT RGB(248, 242, 226)
#define K_SAND RGB(92, 94, 106)
static const uint16_t HUE_C[NHUES] = {RGB(140, 92, 230), RGB(66, 134, 244), RGB(60, 200, 220), RGB(84, 196, 110),
                                      RGB(240, 200, 60), RGB(244, 140, 50), RGB(232, 70, 70), RGB(240, 110, 180),
                                      RGB(150, 150, 158)};
#define K_ACC RGB(255, 84, 164)

static struct {
    int page, hold, dirty, help, browse, bpage, hint;
    uint32_t btn_was, keys_was, btn_at[NB], keys_at;
    char say_a[24], say_b[24];
    uint32_t say_until, autosave, accel_at[NE], last_frame;
    int root_hold, solo, speed_toggle, drift_on, drift[SEATS][4], drift_ph, tape_win;
    int tone_down, crush_on, drive_on, echo_on, brake;
    int saves_cursor, accel_off;
} ui;

static void say(const char *a, const char *b)
{
    int i;
    for (i = 0; i < 23 && a[i]; i++)
        ui.say_a[i] = a[i];
    ui.say_a[i] = 0;
    for (i = 0; i < 23 && b && b[i]; i++)
        ui.say_b[i] = b[i];
    ui.say_b[i] = 0;
    ui.say_until = plat_ms() + 1400u;
    ui.dirty = 1;
}
void ui_say(const char *a, const char *b) { say(a, b); }
static void mark_dirty(void) { ui.dirty = 1; }
int ui_dirty(void) { return ui.dirty != 0; }

static void itoa_u(uint32_t v, char *b)
{
    char t[12];
    int n = 0;
    do {
        t[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    while (n)
        *b++ = t[--n];
    *b = 0;
}
static void itoa_s(int v, char *b)
{
    if (v < 0) {
        *b++ = '-';
        v = -v;
    }
    itoa_u((uint32_t)v, b);
}
static void itoa_w(uint32_t v, char *b, int w)   /* zero padded */
{
    char t[12];
    int n = 0, i;
    itoa_u(v, t);
    while (t[n])
        n++;
    for (i = 0; i < w - n; i++)
        *b++ = '0';
    for (i = 0; i < n; i++)
        *b++ = t[i];
    *b = 0;
}
static void caps(char *d, const char *s, int n)
{
    int i;
    for (i = 0; i < n - 1 && s[i]; i++)
        d[i] = (char)(s[i] >= 'a' && s[i] <= 'z' ? s[i] - 32 : s[i]);
    d[i] = 0;
}
static void cat(char *d, const char *s)
{
    while (*d)
        d++;
    while (*s)
        *d++ = *s++;
    *d = 0;
}

static toy_t *cur(void) { return band_seated(proj.cur) ? &band[proj.cur] : 0; }

static void changed(void)            /* something to keep: save a little later, while stopped */
{
    ui.autosave = plat_ms() + 3000u;
    ui.dirty = 1;
}

/* ------------------------------------------------------------------------------- the knobs --- */
enum { LAY_TOY, LAY_GLO, LAY_EDIT, LAY_FX, LAY_ENV, LAY_LFO, LAY_MENU, LAY_TAPE, LAY_CLOCK };
static const char *const SPAN_N[3] = {"1 bar", "2 bars", "4 bars"};
static int layer(void)
{
    switch (ui.hold) {
    case B_GLO: return LAY_GLO;
    case B_EDIT: return LAY_EDIT;
    case B_FX: return LAY_FX;
    case B_ENV: return LAY_ENV;
    case B_LFO: return LAY_LFO;
    case B_HOME: return LAY_MENU;
    default: return ui.page == PG_TAPE ? LAY_TAPE : ui.page == PG_CLOCK ? LAY_CLOCK : LAY_TOY;
    }
}

static int fx[4] = {0, 0, 64, 54};  /* drive, crush, echo, feedback */
static void fx_send(int i) { en_set(EN_FX, 0, fx[i], i, 0); proj.now.fx[i] = (uint8_t)fx[i]; }

static void knob_label(int k, char *b)
{
    toy_t *t = cur();
    switch (layer()) {
    case LAY_GLO: caps(b, k == 0 ? "Scale" : k == 1 ? "Chords" : k == 2 ? "Tempo" : "Rate", 12); break;
    case LAY_EDIT: caps(b, k == 0 ? "Seed x1000" : k == 1 ? "x100" : k == 2 ? "x10" : "x1", 12); break;
    case LAY_FX: caps(b, k == 0 ? "Drive" : k == 1 ? "Crush" : k == 2 ? "Echo" : "Feedback", 12); break;
    case LAY_ENV: { char s[12] = "SEAT "; s[5] = (char)('1' + mix_seat(k)); s[6] = 0; caps(b, s, 12); break; }
    case LAY_LFO: caps(b, t && k < t->np ? t->p[k].name : "-", 12); cat(b, " ~"); break;
    case LAY_MENU: caps(b, k == 0 ? "Lights" : k == 1 ? "Volume" : "-", 12); break;
    case LAY_TAPE: caps(b, k == 0 ? "Length" : k == 1 ? "Window" : "-", 12); break;
    case LAY_CLOCK: caps(b, k == 0 ? "Dots" : k == 1 ? "Turn" : k == 2 ? "Span" : "-", 12); break;
    default: caps(b, t && k < t->np ? t->p[k].name : "-", 12);
    }
}

static void knob_text(int k, char *b)
{
    toy_t *t = cur();
    b[0] = 0;
    switch (layer()) {
    case LAY_GLO:
        if (k == 0) caps(b, SCALE_NAME[S.scale.value], 12);
        else if (k == 1) { char c[12]; chords_text(c); caps(b, c, 12); }
        else if (k == 2) itoa_s(S.tempo.value, b);
        else param_text(&S.rate, b);
        break;
    case LAY_EDIT:
        if (t) { int d = k == 0 ? 1000 : k == 1 ? 100 : k == 2 ? 10 : 1; itoa_s(t->seed / d % 10, b); }
        break;
    case LAY_FX: itoa_s(fx[k] * 100 / 127, b); cat(b, "%"); break;
    case LAY_ENV: itoa_s(seat_level[mix_seat(k)], b); cat(b, "%"); break;
    case LAY_LFO: itoa_s(ui.drift[proj.cur][k], b); break;
    case LAY_MENU:
        if (k == 0) caps(b, proj.leds == 0 ? "Off" : proj.leds == 1 ? "On" : "Keys", 12);
        else if (k == 1) { itoa_s(S.volume.value, b); cat(b, "%"); }
        break;
    case LAY_TAPE:
        if (k == 0) { itoa_s(band_bars(), b); cat(b, " bars"); }
        else if (k == 1) { itoa_s(ui.tape_win * 16 + 1, b); cat(b, "-"); { char e[8]; itoa_s(ui.tape_win * 16 + 16, e); cat(b, e); } }
        break;
    case LAY_CLOCK:
        if (k == 0) itoa_s(proj.now.dots[proj.cur], b);
        else if (k == 1) itoa_s(proj.now.rot[proj.cur], b);
        else if (k == 2) caps(b, SPAN_N[proj.now.span[proj.cur] % 3], 12);
        break;
    default:
        if (t && k < t->np)
            param_text(&t->p[k], b);
    }
}

static int knob_frac16(int k)
{
    toy_t *t = cur();
    switch (layer()) {
    case LAY_GLO: return k == 0 ? param_frac16(&S.scale) : k == 1 ? S.chords * 65536 / NPROG : k == 2 ? param_frac16(&S.tempo) : param_frac16(&S.rate);
    case LAY_EDIT: return t ? (t->seed / (k == 0 ? 1000 : k == 1 ? 100 : k == 2 ? 10 : 1) % 10) * 65536 / 9 : 0;
    case LAY_FX: return fx[k] * 65536 / 127;
    case LAY_ENV: return seat_level[mix_seat(k)] * 65536 / 100;
    case LAY_LFO: return ui.drift[proj.cur][k] * 65536 / 10;
    case LAY_MENU: return k == 0 ? proj.leds * 65536 / 2 : k == 1 ? S.volume.value * 65536 / 100 : 0;
    case LAY_TAPE: return k == 0 ? (band_bars() - 4) * 65536 / 60 : k == 1 ? ui.tape_win * 65536 / 3 : 0;
    case LAY_CLOCK: return k == 0 ? proj.now.dots[proj.cur] * 65536 / 16 : k == 1 ? proj.now.rot[proj.cur] * 65536 / 15 : k == 2 ? (proj.now.span[proj.cur] % 3) * 65536 / 2 : 0;
    default: return t && k < t->np ? param_frac16(&t->p[k]) : 0;
    }
}

static void turn_knob(int k, int d)
{
    toy_t *t = cur();
    switch (layer()) {
    case LAY_GLO:
        if (k == 0) { int i, at = 0; for (i = 0; i < NDARK; i++) if (DARK_SCALES[i] == S.scale.value) at = i;
            S.scale.value = DARK_SCALES[((at + d) % NDARK + NDARK) % NDARK]; }
        else if (k == 1) S.chords = ((S.chords + d) % (NPROG + 1) + NPROG + 1) % (NPROG + 1);
        else if (k == 2) param_turn(&S.tempo, d);
        else param_turn(&S.rate, d);
        break;
    case LAY_EDIT:
        if (t) { int m = k == 0 ? 1000 : k == 1 ? 100 : k == 2 ? 10 : 1, digit = t->seed / m % 10;
            int nd = ((digit + d) % 10 + 10) % 10; band_seed(proj.cur, t->seed + (nd - digit) * m); }
        break;
    case LAY_FX:
        fx[k] = fx[k] + d * 4 < 0 ? 0 : fx[k] + d * 4 > 127 ? 127 : fx[k] + d * 4;
        fx_send(k);
        break;
    case LAY_ENV: {
        int m = mix_seat(k), v = seat_level[m] + d * 5;
        seat_level[m] = v < 0 ? 0 : v > 100 ? 100 : v;
        break;
    }
    case LAY_LFO:
        ui.drift[proj.cur][k] = ui.drift[proj.cur][k] + d < 0 ? 0 : ui.drift[proj.cur][k] + d > 10 ? 10 : ui.drift[proj.cur][k] + d;
        ui.drift_on = 1;
        break;
    case LAY_MENU:
        if (k == 0) proj.leds = (uint8_t)(((proj.leds + d) % 3 + 3) % 3);
        else if (k == 1) param_turn(&S.volume, d);
        break;
    case LAY_TAPE:
        if (k == 0) { int n = band_bars() + d * 4; proj.now.tape_len = (uint8_t)(n < 4 ? 4 : n > TAPE_MAX ? TAPE_MAX : n); }
        else if (k == 1) { int w = ui.tape_win + d, most = (band_bars() - 1) / 16; ui.tape_win = w < 0 ? 0 : w > most ? most : w; }
        break;
    case LAY_CLOCK:
        if (k == 0) { int n = proj.now.dots[proj.cur] + d; proj.now.dots[proj.cur] = (uint8_t)(n < 0 ? 0 : n > 16 ? 16 : n); }
        else if (k == 1) proj.now.rot[proj.cur] = (uint8_t)(((proj.now.rot[proj.cur] + d) % 16 + 16) % 16);
        else if (k == 2) proj.now.span[proj.cur] = (uint8_t)(((proj.now.span[proj.cur] + d) % 3 + 3) % 3);
        break;
    default:
        band_turn(proj.cur, k, d);
    }
    changed();
}

/* an encoder spun fast moves faster (Playbox's seed knob: tens and hundreds); the host's "spin"
 * turns this off for exact steps */
static int32_t accel(int role, int32_t s)
{
    uint32_t now = plat_ms(), gap = now - ui.accel_at[role];
    ui.accel_at[role] = now;
    if (ui.accel_off || s == 0)
        return s;
    return gap > 110u ? s : gap > 45u ? s * 10 : s * 100;
}

/* ------------------------------------------------------------------------------ the panel --- */
static void seat_set(int i)
{
    proj.cur = (uint8_t)(((i % SEATS) + SEATS) % SEATS);
    ui.dirty = 1;
}

static void toy_pick(int id)       /* this toy into the seat on screen, with the seed it had */
{
    toy_t *t = cur();
    int seed = t ? t->seed : (int)(plat_ms() % 10000u);
    band_seat(proj.cur, ((id % NTOYS) + NTOYS) % NTOYS, seed);
    proj.now.seat[proj.cur].mute = 0;
    say(TOYS[band[proj.cur].id]->name, 0);
    changed();
}

static void shelf_key(int key)     /* the shelf: twenty toys a page on the keys */
{
    int w = key_white[key], b = key_black[key], n = w >= 0 ? w : b >= 0 && b < 4 ? 16 + b : -1, i, c = 0;
    if (n < 0)
        return;
    for (i = 0; i < NTOYS; i++)
        if (TOYS[i]->page == ui.bpage && c++ == n) {
            toy_pick(i);
            ui.browse = 0;
            return;
        }
}

static void speed_set(int pct)
{
    S.speed = pct;
    ui.dirty = 1;
}

static void key_down(int key)
{
    int w = key_white[key], b = key_black[key];
    if (ui.browse) {
        shelf_key(key);
        return;
    }
    if (ui.hold == B_SAVE) {
        if (w >= 0) { char s[8] = "SLOT "; itoa_s(w + 1, s + 5); project_save_slot(w); project_save(); say("SAVED", s); }
        else if (b >= 0 && b < NSCENES) { char s[8] = "BAND "; itoa_s(b + 1, s + 5); project_save_scene(b); project_save(); say("SAVED", s); }
        return;
    }
    if (ui.page == PG_SAVES) {
        if (w >= 0) { say(project_load_slot(w) == 0 ? "LOADED" : "EMPTY", 0); ui.saves_cursor = w; }
        else if (b >= 0 && b < NSCENES) say(project_load_scene(b) == 0 ? "BAND LOADED" : "EMPTY", 0);
        changed();
        return;
    }
    if (ui.hold == B_EDIT) {
        toy_t *t = cur();
        if (w >= 0 && w < 10 && t)
            band_seed(proj.cur, (t->seed * 10 + w) % 10000);
        else if (b == 0)
            band_shake(proj.cur);
        changed();
        return;
    }
    if (ui.hold == B_GLO) {
        if (w >= 0) S.chords = w <= NPROG ? w : NPROG;
        else if (b >= 0 && b < NSCALES) S.scale.value = (int16_t)b;
        changed();
        return;
    }
    if (w >= 0 && ui.page == PG_TAPE) {                   /* a bar of this lane, on or off */
        int bar = ui.tape_win * 16 + w;
        if (bar < band_bars())
            proj.now.tape[proj.cur][bar >> 3] ^= (uint8_t)(1u << (bar & 7));
        changed();
        return;
    }
    if (w >= 0) {
        if (w < 7) ui.root_hold = w;                          /* steer the chord root */
        else if (w == 7) { int i; for (i = 0; i < SEATS; i++) proj.now.seat[i].mute = 0; ui.solo = -1; }
        else if (ui.hold == B_ENV) { ui.solo = w - 8; }       /* ENV held: solo the seat while held */
        else { proj.now.seat[w - 8].mute ^= 1; }
        changed();
        return;
    }
    switch (b) {
    case 0: band_shake(proj.cur); say("NEW SEED", 0); break;
    case 1: { int s = band_undo_seed(proj.cur); if (s >= 0) say("SEED BACK", 0); break; }
    case 2: ui.speed_toggle = ui.speed_toggle == 50 ? 100 : 50; speed_set(ui.speed_toggle); break;
    case 3: ui.speed_toggle = ui.speed_toggle == 200 ? 100 : 200; speed_set(ui.speed_toggle); break;
    case 4: speed_set(0); break;                                      /* freeze while held */
    case 5: ui.brake = 1; break;
    case 6: ui.tone_down = 1; { int i; for (i = 0; i < SEATS; i++) en_set(EN_SEAT_TONE, i, 16, 0, 0); } break;
    case 7: ui.crush_on = 1; en_set(EN_FX, 0, 110, 1, 0); break;
    case 8: ui.drive_on = 1; en_set(EN_FX, 0, 100, 0, 0); break;
    case 9: ui.echo_on = 1; en_set(EN_FX, 0, 127, 3, 0); en_set(EN_ECHO, 0, 127, 0, 0); break;
    case 10: settings_calm(); ui.speed_toggle = 100; ui.solo = -1; ui.drift_on = 0; fx_send(0); fx_send(1); fx_send(3);
             en_set(EN_ECHO, 0, 64, 0, 0); say("CALM", 0); break;
    default: break;
    }
    changed();
}

static void key_up(int key)
{
    int w = key_white[key], b = key_black[key];
    if (w >= 0 && w < 7 && ui.root_hold == w)
        ui.root_hold = -1;
    if (w >= 8 && ui.solo == w - 8)
        ui.solo = -1;
    switch (b) {
    case 4: speed_set(ui.speed_toggle ? ui.speed_toggle : 100); break;
    case 5: ui.brake = 0; speed_set(ui.speed_toggle ? ui.speed_toggle : 100); break;
    case 6: ui.tone_down = 0; { int i; for (i = 0; i < SEATS; i++) en_set(EN_SEAT_TONE, i, 64, 0, 0); } break;
    case 7: ui.crush_on = 0; fx_send(1); break;
    case 8: ui.drive_on = 0; fx_send(0); break;
    case 9: ui.echo_on = 0; fx_send(3); en_set(EN_ECHO, 0, 64, 0, 0); break;
    default: break;
    }
    ui.dirty = 1;
}

static void button_tap(int b)      /* pressed and released without a layer being used */
{
    switch (b) {
    case B_PLAY: band_play(!band_playing); if (!band_playing) changed(); break;
    case B_HOME: ui.browse = !ui.browse; ui.help = 0; break;
    case B_OCTDN: if (ui.browse) ui.bpage = (ui.bpage + NSHELF - 1) % NSHELF; else seat_set(proj.cur - 1); break;
    case B_OCTUP: if (ui.browse) ui.bpage = (ui.bpage + 1) % NSHELF; else seat_set(proj.cur + 1); break;
    case B_REC: band_shake(proj.cur); say("NEW SEED", 0); changed(); break;
    case B_SEQ:                 /* the arrangement: Band, Tape, Clock; and its page */
        proj.now.mode = (uint8_t)((proj.now.mode + 1) % NMODES);
        ui.page = proj.now.mode == MODE_TAPE ? PG_TAPE : proj.now.mode == MODE_CLOCK ? PG_CLOCK : PG_BAND;
        say(MODE_NAME[proj.now.mode], 0);
        changed();
        break;
    case B_ARP: proj.now.seat[proj.cur].mute ^= 1; say(proj.now.seat[proj.cur].mute ? "MUTED" : "ON", 0); changed(); break;
    case B_SEL: { toy_t *t = cur(); if (t) toy_scale_next(t); say("SCALE", SCALE_NAME[S.scale.value]); changed(); break; }
    case B_SAVE: project_save(); say("SAVED", 0); break;
    case B_GLO: ui.help = ui.help ? 0 : 1; break;
    default: break;
    }
    ui.dirty = 1;
}

static void button_held(int b)     /* held past the tap time */
{
    if (b == B_REC) {
        if (ui.page == PG_TAPE || ui.page == PG_CLOCK) {
            band_fill_arrangement(plat_ms() * 2654435761u);
            say(ui.page == PG_TAPE ? "RANDOM TAPE" : "RANDOM CLOCK", 0);
        } else {
            band_fill(plat_ms() * 2654435761u);
            say("RANDOM BAND", 0);
        }
        changed();
    }
}

#define TAP_MS 350
static uint8_t used_as_layer[NB];

static void input(void)
{
    uint32_t btn = plat_buttons(), keys = plat_keys(), now = plat_ms(), went, came;
    int b, k;
    int32_t e;
    /* buttons */
    came = btn & ~ui.btn_was;
    went = ~btn & ui.btn_was;
    for (b = 0; b < NB; b++) {
        if (came & (1u << b)) {
            ui.btn_at[b] = now;
            used_as_layer[b] = 0;
            if (b == B_GLO || b == B_EDIT || b == B_FX || b == B_ENV || b == B_LFO || b == B_HOME || b == B_SAVE)
                ui.hold = b;
            ui.dirty = 1;
        }
        if (went & (1u << b)) {
            if (ui.hold == b)
                ui.hold = L_NONE;
            if (!used_as_layer[b] && now - ui.btn_at[b] < TAP_MS)
                button_tap(b);
            else if (!used_as_layer[b] && b == B_REC)
                button_held(b);
            ui.dirty = 1;
        }
    }
    ui.btn_was = btn;
    /* keys */
    came = keys & ~ui.keys_was;
    went = ~keys & ui.keys_was;
    for (k = 0; k < NKEYS; k++) {
        if (came & (1u << k)) {
            if (ui.hold >= 0)
                used_as_layer[ui.hold] = 1;
            key_down(k);
        }
        if (went & (1u << k))
            key_up(k);
    }
    ui.keys_was = keys;
    /* encoders */
    e = plat_enc(EN_SELECT);
    if (e) {
        if (ui.hold == B_HOME) {
            proj.leds = (uint8_t)(((proj.leds + e) % 3 + 3) % 3);
            used_as_layer[B_HOME] = 1;
        } else {
            ui.page = ((ui.page + e) % NPAGES + NPAGES) % NPAGES;
            ui.browse = ui.help = 0;
        }
        ui.dirty = 1;
    }
    e = plat_enc(EN_ALGO);
    if (e) {
        if (ui.page == PG_BAND || ui.page == PG_TAPE || ui.page == PG_CLOCK)
            seat_set(proj.cur + e);
        else if (ui.page == PG_SAVES)
            ui.saves_cursor = ((ui.saves_cursor + e) % NSLOTS + NSLOTS) % NSLOTS;
        else if (ui.help)
            ui.help = ((ui.help - 1 + e) % 3 + 3) % 3 + 1;
        else
            toy_pick((cur() ? cur()->id : 0) + e);
        ui.dirty = 1;
    }
    e = plat_enc(EN_PRESET);
    if (e && cur()) {
        band_seed(proj.cur, ((cur()->seed + accel(EN_PRESET, e)) % 10000 + 10000) % 10000);
        changed();
    }
    for (k = 0; k < 4; k++) {
        e = plat_enc(EN_K1 + k);
        if (e) {
            if (ui.hold >= 0)
                used_as_layer[ui.hold] = 1;
            turn_knob(k, e);
        }
    }
}

/* ------------------------------------------------------------------------------- the clock --- */
static void drift(void)            /* the slow wobble on the knobs (LFO): a step every 250 ms, each knob
                                      wandering up to Drift clicks either side of where it was, as if a
                                      hand turned it back and forth */
{
    static uint32_t last;
    static int8_t off[SEATS][4], dir[SEATS][4];
    uint32_t now = plat_ms();
    int k, s = proj.cur;
    toy_t *t = cur();
    if (!t || now - last < 250u)
        return;
    last = now;
    for (k = 0; k < 4 && k < t->np; k++) {
        int depth = ui.drift_on ? ui.drift[s][k] : 0, step;
        if (!dir[s][k])
            dir[s][k] = 1;
        if (depth == 0 && off[s][k] == 0)
            continue;
        if (depth == 0)                 /* drift off: back to where it was */
            step = off[s][k] > 0 ? -1 : 1;
        else {
            if (off[s][k] >= depth) dir[s][k] = -1;
            if (off[s][k] <= -depth) dir[s][k] = 1;
            step = dir[s][k] * ((ui.drift_ph++ % (12 - depth)) == 0 ? 1 : 0);
        }
        if (step) {
            int before = t->p[k].value;
            toy_turn(t, k, step);
            off[s][k] = (int8_t)(off[s][k] + (t->p[k].value - before));
            ui.dirty = 1;
        }
    }
}

static void brake(void)
{
    if (ui.brake && S.speed > 0) {
        static uint32_t last;
        uint32_t now = plat_ms();
        if (now - last > 40u) {
            last = now;
            S.speed = S.speed > 4 ? S.speed - 4 : 0;
        }
    }
}

static void solo(void) { band_solo = ui.solo; }

/* ------------------------------------------------------------------------------- drawing --- */
static uint32_t hash(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

static void text_c(int32_t cx, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    cv_text(cx - text_w(f, s) / 2, y, f, s, c);
}

static uint16_t seat_colour(int i) { return band[i].c ? HUE_C[band[i].c->hue] : K_SAND; }

static void draw_header(void)      /* 0..24: the seat and its toy, the seed, the tempo and scale */
{
    char b[32], s[12];
    toy_t *t = cur();
    cv_begin(240, 24, K_PANEL);
    cv_rect(0, 22, 240, 2, K_LINE);
    cv_rect(0, 0, 4, 22, seat_colour(proj.cur));
    b[0] = (char)('1' + proj.cur);
    b[1] = ' ';
    b[2] = 0;
    caps(b + 2, t ? t->c->name : "empty seat", 20);
    cv_text(10, 3, &FONT_S, b, K_TEXT);
    if (t) {
        b[0] = '#';
        itoa_w(t->seed, b + 1, 4);
        cv_text(150, 3, &FONT_S, b, HUE_C[t->c->hue]);
    }
    if (proj.now.mode == MODE_BAND) {
        itoa_s(S.tempo.value, s);
        cv_text(206, 3, &FONT_S, s, K_DIM);
    } else {
        itoa_s((S.band_steps / BAR) % band_bars() + 1, s);
        cv_text(196, 3, &FONT_XS, proj.now.mode == MODE_TAPE ? "T" : "C", K_ACC);
        cv_text(206, 3, &FONT_S, s, K_DIM);
    }
    if (ui.say_until > plat_ms()) {
        int32_t w = text_w(&FONT_S, ui.say_a) + 6;
        cv_rect(240 - w - 2, 0, w + 2, 22, K_ACC);
        cv_text(240 - w + 1, 3, &FONT_S, ui.say_a, K_TEXT);
    }
    cv_blit(0, 0);
}

static void draw_roll(void)        /* the toy page: what the seat played lately, scrolling */
{
    uint32_t now = plat_ms(), i;
    toy_t *t = cur();
    int x;
    cv_begin(240, 144, K_BG);
    for (x = 0; x < 240; x += 30)
        cv_rect(x, 0, 1, 144, K_LINE);
    cv_rect(0, 72, 240, 1, K_LINE);
    for (i = 0; i < NOTE_LOG; i++) {
        const note_log_t *l = &note_log[i];
        uint32_t age;
        int px, py, w;
        if (!l->at || l->seat != proj.cur)
            continue;
        age = now - l->at;
        if (age > 6000u)
            continue;
        px = 232 - (int)(age * 232u / 6000u);
        if (l->role == ROLE_DRUMS || l->role == ROLE_TEXTURE) {
            py = 120 + (l->voice % 4) * 5;
            w = 6;
        } else {
            int d = l->degree + 10;
            py = 128 - (d < 0 ? 0 : d > 36 ? 36 : d) * 3;
            w = 10;
        }
        cv_rect(px - w, py, w, 4, age < 150u ? K_TEXT : seat_colour(proj.cur));
    }
    if (t) {
        char b[8];
        int pos = (S.band_steps % BAR) * 236 / BAR;
        cv_rect(2 + pos, 134, 236 / BAR - 2, 6, band_playing ? K_TEXT : K_SAND);
        itoa_s(S.band_steps / BAR + 1, b);
        cv_text(4, 4, &FONT_XS, b, K_DIM);
        if (proj.now.seat[proj.cur].mute)
            text_c(120, 60, &FONT_B, "MUTED", K_DIM);
        if (S.speed != 100) {
            char s[12];
            itoa_s(S.speed, s);
            cat(s, "%");
            cv_text(200, 4, &FONT_XS, s, K_ACC);
        }
    } else {
        text_c(120, 48, &FONT_B, "EMPTY SEAT", K_DIM);
        text_c(120, 76, &FONT_XS, "TURN ALGORITHM, OR HOME", K_DIM);
    }
    cv_blit(0, 24);
}

/* a singer: a little face that opens its mouth for a moment when its seat sounds a note */
static void draw_singer(int x, int y, uint16_t c, int open, int dim)
{
    uint16_t face = dim ? K_SAND : c;
    cv_rect(x + 3, y, 8, 2, face);
    cv_rect(x + 1, y + 2, 12, 10, face);
    cv_rect(x + 3, y + 12, 8, 2, face);
    cv_rect(x + 4, y + 4, 2, 2, K_BG);                  /* eyes */
    cv_rect(x + 8, y + 4, 2, 2, K_BG);
    cv_rect(x + 4, y + 8, 6, open ? 4 : 1, K_BG);       /* mouth */
}

static int sang(int s, uint32_t now, uint32_t within)  /* did seat s sound a note lately? */
{
    uint32_t i;
    for (i = 0; i < NOTE_LOG; i++)
        if (note_log[i].at && note_log[i].seat == (uint32_t)s && now - note_log[i].at < within)
            return 1;
    return 0;
}

static void draw_band(void)        /* the band page: eight seats, their toys, singers and their state */
{
    uint32_t now = plat_ms();
    int s, bar = S.band_steps / BAR;
    cv_begin(240, 144, K_BG);
    for (s = 0; s < SEATS; s++) {
        int y = s * 18, on = band_bar_on(s, bar);
        char b[24];
        uint16_t c = seat_colour(s);
        int quiet = proj.now.seat[s].mute || S.cuts[s] || (band_solo >= 0 && band_solo != s) || !on;
        if (s == proj.cur)
            cv_rect(0, y, 240, 17, K_PANEL);
        cv_rect(2, y + 2, 3, 13, c);
        if (band[s].c)
            draw_singer(8, y + 2, c, sang(s, now, 160u), quiet);
        b[0] = (char)('1' + s);
        b[1] = 0;
        cv_text(26, y + 1, &FONT_XS, b, K_DIM);
        caps(b, band[s].c ? band[s].c->name : "empty", 16);
        cv_text(38, y + 1, &FONT_XS, b, quiet ? K_DIM : K_TEXT);
        if (band[s].c) {
            b[0] = '#';
            itoa_w(band[s].seed, b + 1, 4);
            cv_text(136, y + 1, &FONT_XS, b, c);
            itoa_s(seat_level[s], b);
            cat(b, "%");
            cv_text(176, y + 1, &FONT_XS, b, K_DIM);
            if (S.cuts[s])
                cv_text(208, y + 1, &FONT_XS, "CUT", K_ACC);
            else if (proj.now.seat[s].mute)
                cv_text(208, y + 1, &FONT_XS, "MUTE", K_DIM);
            else if (band_solo >= 0 && band_solo != s)
                cv_text(208, y + 1, &FONT_XS, "SOLO", K_DIM);
            else if (!on)
                cv_text(208, y + 1, &FONT_XS, "OFF", K_DIM);
        }
    }
    cv_blit(0, 24);
}

static void draw_tape(void)        /* eight lanes, sixteen bars of them: a cell per bar, lit where it plays */
{
    int s, bar, len = band_bars(), now_bar = S.band_steps / BAR, win = ui.tape_win * 16;
    char b[12];
    cv_begin(240, 144, K_BG);
    for (bar = 0; bar < 16; bar++) {
        int x = 24 + bar * 13;
        if (win + bar >= len)
            break;
        itoa_s(win + bar + 1, b);
        cv_text(x + 1, 0, &FONT_XS, b, (win + bar) % len == now_bar % len && band_playing ? K_TEXT : K_DIM);
    }
    for (s = 0; s < SEATS; s++) {
        int y = 16 + s * 16;
        uint16_t c = seat_colour(s);
        b[0] = (char)('1' + s);
        b[1] = 0;
        cv_text(4, y, &FONT_XS, b, s == proj.cur ? K_TEXT : K_DIM);
        if (s == proj.cur)
            cv_rect(0, y - 1, 2, 15, K_ACC);
        cv_rect(14, y + 3, 6, 8, band[s].c ? c : K_LINE);
        for (bar = 0; bar < 16; bar++) {
            int x = 24 + bar * 13, abs_bar = win + bar, on;
            if (abs_bar >= len)
                break;
            on = (proj.now.tape[s][abs_bar >> 3] >> (abs_bar & 7)) & 1;
            cv_rect(x, y, 12, 13, on ? (band[s].c ? c : K_DIM) : K_PANEL);
            if (band_playing && abs_bar == now_bar % len)
                cv_rect(x, y + 11, 12, 2, K_TEXT);
        }
    }
    cv_blit(0, 24);
}

static void draw_clock(void)       /* eight rings of sixteen dots; the hand goes round once a span */
{
    static const int8_t CX[16] = {0, 10, 18, 24, 26, 24, 18, 10, 0, -10, -18, -24, -26, -24, -18, -10};
    static const int8_t CY[16] = {-26, -24, -18, -10, 0, 10, 18, 24, 26, 24, 18, 10, 0, -10, -18, -24};
    int s, now_bar = S.band_steps / BAR;
    cv_begin(240, 144, K_BG);
    for (s = 0; s < SEATS; s++) {
        int cx = 30 + (s % 4) * 60, cy = 36 + (s / 4) * 72, d, per = 1 << (proj.now.span[s] % 3);
        uint32_t m = euclid16(proj.now.dots[s] > 16 ? 16 : proj.now.dots[s], proj.now.rot[s] & 15);
        int at = (now_bar / per) & 15;
        uint16_t c = band[s].c ? seat_colour(s) : K_LINE;
        char b[4];
        if (s == proj.cur)
            cv_rect(cx - 29, cy - 29, 58, 58, K_PANEL);
        for (d = 0; d < 16; d++) {
            int on = (int)(m >> d) & 1, x = cx + CX[d], y = cy + CY[d];
            cv_rect(x - (on ? 2 : 1), y - (on ? 2 : 1), on ? 5 : 3, on ? 5 : 3, on ? c : K_LINE);
            if (band_playing && d == at)
                cv_rect(x - 3, y - 3, 7, 7, on ? K_TEXT : K_DIM);
        }
        b[0] = (char)('1' + s);
        b[1] = 0;
        cv_text(cx - 3, cy - 7, &FONT_XS, b, K_DIM);
    }
    cv_blit(0, 24);
}

static void draw_saves(void)
{
    int i;
    cv_begin(240, 144, K_BG);
    for (i = 0; i < NSLOTS; i++) {
        int x = (i % 4) * 60, y = (i / 4) * 28;
        char b[16];
        if (i == ui.saves_cursor)
            cv_rect(x, y, 59, 27, K_PANEL);
        itoa_s(i + 1, b);
        cv_text(x + 4, y + 2, &FONT_XS, b, K_DIM);
        if (proj.slot[i].toy >= 0) {
            caps(b, TOYS[proj.slot[i].toy]->name, 11);
            cv_text(x + 4, y + 13, &FONT_XS, b, HUE_C[TOYS[proj.slot[i].toy]->hue]);
        }
    }
    cv_rect(0, 113, 240, 1, K_LINE);
    for (i = 0; i < NSCENES; i++) {
        int x = i * 60, j, any = 0;
        char b[16] = "BAND ";
        for (j = 0; j < SEATS; j++)
            any |= proj.scene[i].seat[j].toy >= 0;
        itoa_s(i + 1, b + 5);
        cv_text(x + 4, 118, &FONT_XS, b, any ? K_TEXT : K_DIM);
    }
    text_c(120, 132, &FONT_XS, "KEYS LOAD. HOLD SAVE + A KEY SAVES", K_DIM);
    cv_blit(0, 24);
}

static void draw_shelf(void)       /* the toy box: twenty toys of this page, one per key */
{
    int i, n = 0;
    char b[20];
    cv_begin(240, 144, K_BG);
    for (i = 0; i < NTOYS; i++) {
        int x, y;
        if (TOYS[i]->page != ui.bpage)
            continue;
        x = (n % 3) * 80;
        y = (n / 3) * 18;
        if (cur() && cur()->id == i)
            cv_rect(x, y, 79, 17, K_PANEL);
        itoa_s(n + 1, b);
        cv_text(x + 2, y + 1, &FONT_XS, b, K_DIM);
        caps(b, TOYS[i]->name, 16);
        if (b[11]) {                            /* too wide for the column: the first word(s) that fit */
            int c = 11;
            while (c > 0 && b[c] != ' ')
                c--;
            b[c > 0 ? c : 11] = 0;
        }
        cv_text(x + 16, y + 1, &FONT_XS, b, HUE_C[TOYS[i]->hue]);
        n++;
    }
    caps(b, SHELF_NAME[ui.bpage], 14);
    cv_text(4, 128, &FONT_XS, b, K_TEXT);
    cv_text(96, 128, &FONT_XS, "KEYS 1-20 PICK. OCT-/+ PAGE", K_DIM);
    cv_blit(0, 24);
}

static const char *const HELP[3][9] = {
    {"KNOBS   the toy's four knobs", "ALGO    the toy, PRESETS the seed", "SELECT  Toy / Band / Saves",
     "PLAY    the band; REC a new seed", "OCT -/+ the seat; HOME the shelf", "SEL     next scale; ARP mute",
     "SEQ     band / tape / clock", "REC held: a random band, or tape", "GLO     this help; ALGO turns the page"},
    {"HOLD GLO   scale chords tempo rate", "HOLD EDIT  the seed's four digits", "HOLD FX    drive crush echo feedback",
     "HOLD ENV   the mixer (this half)", "HOLD LFO   drift on the four knobs", "HOLD SAVE  + key: save there",
     "HOLD HOME  lights, volume", "GLO + key  chords (white) scale (black)", "EDIT + key type the seed"},
    {"WHITE 1-7  steer the chord root", "WHITE 8    everyone back in", "WHITE 9-16 mute seats 1-8",
     "ENV + 9-16  solo a seat (held)", "BLACK 1 dice   2 undo   3 half", "BLACK 4 double 5 freeze 6 brake",
     "BLACK 7 filter 8 crush 9 drive", "BLACK 10 echo throw  11 calm", "TAPE page: keys toggle bars"},
};
static void draw_help(void)
{
    int i;
    cv_begin(240, 144, K_BG);
    for (i = 0; i < 9; i++)
        cv_text(4, 2 + i * 15, &FONT_XS, HELP[ui.help - 1][i], i % 2 ? K_DIM : K_TEXT);
    cv_blit(0, 24);
}

static void draw_knobs(void)       /* 168..240: four knobs, each a label, a value and a bar */
{
    int k;
    const char *lay = layer() == LAY_GLO ? "GLOBAL" : layer() == LAY_EDIT ? "SEED" : layer() == LAY_FX ? "EFFECTS"
                    : layer() == LAY_ENV ? "MIXER" : layer() == LAY_LFO ? "DRIFT" : layer() == LAY_MENU ? "HOME" : 0;
    cv_begin(240, 72, K_PANEL);
    cv_rect(0, 0, 240, 2, K_LINE);
    for (k = 0; k < 4; k++) {
        int x = k * 60;
        char b[16];
        uint16_t c = cur() ? HUE_C[cur()->c->hue] : K_DIM;
        if (lay)
            c = K_ACC;
        knob_label(k, b);
        cv_text(x + 5, 6, &FONT_XS, b, K_DIM);
        knob_text(k, b);
        cv_text(x + 5, 24, &FONT_S, b, K_TEXT);
        cv_rect(x + 5, 52, 50, 4, K_LINE);
        cv_rect(x + 5, 52, (knob_frac16(k) * 50) >> 16, 4, c);
        if (k)
            cv_rect(x, 8, 1, 56, K_LINE);
    }
    if (lay)
        cv_text(4, 60, &FONT_XS, lay, K_ACC);
    cv_blit(0, 168);
}

static void draw_all(void)
{
    draw_header();
    if (ui.browse)
        draw_shelf();
    else if (ui.help)
        draw_help();
    else if (ui.page == PG_BAND)
        draw_band();
    else if (ui.page == PG_TAPE)
        draw_tape();
    else if (ui.page == PG_CLOCK)
        draw_clock();
    else if (ui.page == PG_SAVES)
        draw_saves();
    else
        draw_roll();
    draw_knobs();
}

/* ------------------------------------------------------------------------------- the lights --- */
static void leds(void)
{
    uint32_t b = 0, k = 0, now = plat_ms(), i;
    int s;
    if (band_playing)
        b |= 1u << B_PLAY;
    if (ui.hold >= 0)
        b |= 1u << ui.hold;
    if (ui.browse)
        b |= 1u << B_HOME;
    if (proj.now.mode != MODE_BAND)
        b |= 1u << B_SEQ;
    if (proj.now.seat[proj.cur].mute)
        b |= 1u << B_ARP;
    if (ui.browse) {
        int n = 0;
        for (i = 0; i < (uint32_t)NTOYS; i++)
            if (TOYS[i]->page == ui.bpage) {
                k |= 1u << (n < 16 ? WHITE[n] : BLACK[n - 16]);
                n++;
            }
    } else {
        if (ui.page == PG_TAPE) {
            int bar;
            for (bar = 0; bar < 16; bar++)
                if (ui.tape_win * 16 + bar < band_bars() && band_bar_on(proj.cur, ui.tape_win * 16 + bar))
                    k |= 1u << WHITE[bar];
        } else {
            for (s = 0; s < SEATS; s++)
                if (band[s].c && !proj.now.seat[s].mute && (ui.solo < 0 || ui.solo == s))
                    k |= 1u << WHITE[8 + s];
        }
        if (ui.root_hold >= 0)
            k |= 1u << WHITE[ui.root_hold];
        if (ui.speed_toggle == 50) k |= 1u << BLACK[2];
        if (ui.speed_toggle == 200) k |= 1u << BLACK[3];
        if (ui.drift_on) k |= 1u << BLACK[0];
        for (i = 0; i < NOTE_LOG; i++) {        /* the notes light the keys */
            const note_log_t *l = &note_log[i];
            if (l->at && now - l->at < 120u && !proj.now.seat[l->seat].mute) {
                if (l->role == ROLE_DRUMS || l->role == ROLE_TEXTURE)
                    k |= 1u << BLACK[(l->voice + 4) % 11];
                else
                    k |= 1u << WHITE[(((l->degree % 7) + 7) % 7) + (l->seat % 2 ? 0 : 0)];
            }
        }
    }
    if (proj.leds == 0)
        k = 0;
    plat_glow(proj.leds == 1);
    plat_leds(b, k);
}

/* --------------------------------------------------------------------------------- frames --- */
static void autosave(void)
{
    if (ui.autosave && plat_ms() > ui.autosave && !band_playing) {
        ui.autosave = 0;
        project_save();
    }
}

void ui_init(void)
{
    int i;
    for (i = 0; i < 27; i++)
        key_white[i] = key_black[i] = -1;
    for (i = 0; i < 16; i++)
        key_white[WHITE[i]] = (int8_t)i;
    for (i = 0; i < 11; i++)
        key_black[BLACK[i]] = (int8_t)i;
    ui.hold = L_NONE;
    ui.root_hold = -1;
    ui.solo = -1;
    ui.speed_toggle = 100;
    ui.btn_was = plat_buttons();
    ui.keys_was = plat_keys();
    for (i = 0; i < 4; i++)
        fx[i] = proj.now.fx[i];
    for (i = 0; i < 4; i++)
        fx_send(i);
    ui.dirty = 1;
    draw_all();
    leds();
}

void ui_frame(void)
{
    input();
    band_tick(plat_ms());
    drift();
    brake();
    solo();
    S.root_hold = ui.root_hold;
    autosave();
    draw_all();
    leds();
    ui.dirty = 0;
}

void ui_input_only(void)
{
    input();
    band_tick(plat_ms());
    solo();
    S.root_hold = ui.root_hold;
}
