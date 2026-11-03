/* SPDX-License-Identifier: GPL-3.0-only */
/* The band: four seats, each holding a Witchbox toy, playing together on one clock with one set of
 * chords (Playbox's band.py). The toys run in fixed 5 ms slices of time, in the order they fall
 * due, so the same seeds give the same notes however the main loop is timed. Part of the unity build. */

toy_t band[SEATS] EN_POOL;
uint8_t band_playing;
static int band_acc, band_step_ms = 125, band_pending;
static uint32_t band_last_ms;
static uint16_t seed_was[SEATS];     /* the seed before the last Random, for the undo key */

int band_solo = -1;                 /* a seat soloed from the keys, else -1 */
note_log_t note_log[NOTE_LOG];
uint32_t note_log_n;

static void roles(void)
{
    int i, drummer = 0;
    for (i = 0; i < SEATS; i++) {
        S.roles[i] = band[i].c ? band[i].c->role : ROLE_NONE;
        drummer |= S.roles[i] == ROLE_DRUMS;
    }
    S.drummer = drummer;
}

void band_init(void)
{
    int i;
    settings_init();
    sound_init();
    S.band = 1;                     /* the band's chords and clock rule, always */
    for (i = 0; i < SEATS; i++) {
        band[i].c = 0;
        band[i].seat = i;
        seat_level[i] = 80;
        proj.now.seat[i].level = 80;
    }
    roles();
}

int band_seated(int i) { return i >= 0 && i < SEATS && band[i].c != 0; }

void band_seat(int i, int toy, int seed)
{
    toy_t *t;
    if (i < 0 || i >= SEATS)
        return;
    t = &band[i];
    if (t->c) {
        toy_close(t);
        t->c = 0;
    }
    if (toy >= 0 && toy < NTOYS) {
        toy_open(t, toy, seed);
        t->seat = i;
        if (band_playing) {
            toy_play(t, 1);
            t->steps = S.band_steps;
        }
    }
    roles();
    seed_was[i] = (uint16_t)seed;
}

void band_play(int on)
{
    int i;
    band_playing = (uint8_t)on;
    band_acc = 0;
    if (on)
        S.band_steps = 0;
    else
        sound_hush();
    for (i = 0; i < SEATS; i++)
        if (band[i].c)
            toy_play(&band[i], on);
    if (!on)
        settings_calm();
}

static void slice(int ms)          /* one fixed slice of time for the band and every seat */
{
    int i;
    if (band_playing && S.speed > 0) {
        band_step_ms = 1500000 / (S.tempo.value * S.speed);
        band_acc += ms;
        if (band_acc >= band_step_ms) {
            int n = band_acc / band_step_ms;
            band_acc -= n * band_step_ms;
            S.band_steps += n;
        }
    }
    for (i = 0; i < SEATS; i++)
        if (band[i].c)
            toy_tick(&band[i], ms);
}

void band_tick(uint32_t now_ms)
{
    uint32_t gone = now_ms - band_last_ms;
    band_last_ms = now_ms;
    if (gone > 200u)                /* a long pause (saving to flash): do not run it all at once */
        gone = 200u;
    band_pending += (int)gone;
    while (band_pending >= 5) {
        band_pending -= 5;
        slice(5);
    }
}

void band_shake(int i)
{
    if (!band_seated(i))
        return;
    seed_was[i] = band[i].seed;
    toy_shake(&band[i], dice_fresh(plat_ms() * 2654435761u + (uint32_t)i * 7919u));
    if (band_playing)
        band[i].steps = S.band_steps;
}

int band_undo_seed(int i)
{
    int s;
    if (!band_seated(i))
        return -1;
    s = seed_was[i];
    seed_was[i] = band[i].seed;
    band[i].seed = (uint16_t)s;
    toy_restart(&band[i]);
    if (band_playing)
        band[i].steps = S.band_steps;
    return s;
}

void band_seed(int i, int seed)
{
    if (!band_seated(i))
        return;
    seed_was[i] = band[i].seed;
    band[i].seed = (uint16_t)seed;
    toy_restart(&band[i]);
    if (band_playing)
        band[i].steps = S.band_steps;
}

void band_turn(int i, int k, int d)
{
    if (band_seated(i))
        toy_turn(&band[i], k, d);
}

void band_fill(uint32_t entropy)   /* a random band: a drum, a bass, a chord and a lead toy */
{
    static const uint8_t want[SEATS] = {ROLE_DRUMS, ROLE_BASS, ROLE_CHORDS, ROLE_LEAD, ROLE_VOICE, ROLE_TEXTURE, ROLE_LEAD, ROLE_CHORDS};
    static dice_t d;
    int i;
    dice_start(&d, (int)(entropy & 0xFFFFu));
    dice_use(&d);
    for (i = 0; i < SEATS; i++) {
        int pick = -1, n = 0, j;
        for (j = 0; j < NTOYS; j++)
            if (TOYS[j]->role == want[i])
                n++;
        if (n) {
            int k = dice_pick(n);
            for (j = 0; j < NTOYS; j++)
                if (TOYS[j]->role == want[i] && k-- == 0)
                    pick = j;
        }
        {
            int seed = dice_pick(10000);
            band_seat(i, pick, seed);
            dice_use(&d);           /* seating a toy switched the dice to its own */
        }
        proj.now.seat[i].mute = 0;
    }
}

/* ---------------------------------------------------------------------- the arrangements --- */
uint32_t euclid16(int k, int rot)
{
    uint32_t m = 0;
    int i;
    for (i = 0; i < 16; i++)
        if ((i * k) % 16 < k)
            m |= 1u << ((i + rot) & 15);
    return k >= 16 ? 0xFFFFu : m;
}

int band_bars(void)
{
    if (proj.now.mode == MODE_TAPE)
        return proj.now.tape_len < 4 ? 4 : proj.now.tape_len;
    if (proj.now.mode == MODE_CLOCK) {
        int i, most = 16;
        for (i = 0; i < SEATS; i++) {
            int n = 16 << (proj.now.span[i] % 3);
            if (n > most)
                most = n;
        }
        return most;
    }
    return 0;
}

int band_bar_on(int i, int bar)
{
    const scene_t *sc = &proj.now;
    if (i < 0 || i >= SEATS)
        return 0;
    if (sc->mode == MODE_TAPE) {
        int len = sc->tape_len < 4 ? 4 : sc->tape_len > TAPE_MAX ? TAPE_MAX : sc->tape_len, b = bar % len;
        return (sc->tape[i][b >> 3] >> (b & 7)) & 1;
    }
    if (sc->mode == MODE_CLOCK) {
        int per = 1 << (sc->span[i] % 3), dot = (bar / per) & 15;
        return (int)(euclid16(sc->dots[i] > 16 ? 16 : sc->dots[i], sc->rot[i] & 15) >> dot) & 1;
    }
    return 1;
}

void band_fill_arrangement(uint32_t entropy)
{
    static dice_t d;
    int i, b;
    dice_start(&d, (int)(entropy & 0xFFFFu));
    dice_use(&d);
    if (proj.now.mode == MODE_TAPE) {
        int len = proj.now.tape_len < 4 ? 4 : proj.now.tape_len;
        for (i = 0; i < SEATS; i++) {
            int on = 0, run = 0;
            for (b = 0; b < TAPE_MAX / 8; b++)
                proj.now.tape[i][b] = 0;
            for (b = 0; b < len; b++) {         /* clips of a few bars, gaps of a few bars */
                if (run == 0) {
                    on = !on;
                    run = (on ? 2 : 1) + dice_pick(on ? 6 : 4);
                    if (b == 0 && !band_seated(i))
                        on = 0;
                }
                run--;
                if (on && band_seated(i))
                    proj.now.tape[i][b >> 3] |= (uint8_t)(1u << (b & 7));
            }
        }
    } else if (proj.now.mode == MODE_CLOCK) {
        for (i = 0; i < SEATS; i++) {
            proj.now.dots[i] = (uint8_t)(band_seated(i) ? 3 + dice_pick(12) : 0);
            proj.now.rot[i] = (uint8_t)dice_pick(16);
            proj.now.span[i] = (uint8_t)dice_pick(3);
        }
    }
}

/* every toy's note ends here: a muted seat, a seat a conductor has cut, a seat whose lane or ring is
 * off this bar, and notes thinned out by a low intensity go no further */
void heard(int role, int degree, int voice, int module, int seat, int hold)
{
    note_log_t *l;
    if (seat < 0 || seat >= SEATS)
        seat = 0;
    if (proj.now.seat[seat].mute || S.cuts[seat] || (band_solo >= 0 && band_solo != seat))
        return;
    if (!band_bar_on(seat, S.band_steps / BAR))
        return;
    if (S.levels[seat] < 100 && !dice_mul_lt(100, S.levels[seat]))
        return;
    sound_play(role, degree, voice, module, seat, hold);
    if (role == ROLE_DRUMS && voice == 0)
        S.kicks++;
    else if (role == ROLE_LEAD) {
        S.lead = degree;
        S.has_lead = 1;
        S.leads++;
    }
    l = &note_log[note_log_n++ % NOTE_LOG];
    l->seat = (uint8_t)seat;
    l->voice = (uint8_t)(voice < 0 ? 0 : voice);
    l->degree = (int8_t)degree;
    l->role = (uint8_t)role;
    l->at = plat_ms();
}
