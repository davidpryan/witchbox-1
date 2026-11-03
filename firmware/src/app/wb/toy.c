/* SPDX-License-Identifier: GPL-3.0-only */
/* The toy: its knobs, its clock, and how it is opened, restarted and shaken (Playbox's toy.py and
 * dark.py, with v0.12's seeds). Nothing here draws; ui.c does that. */
#include "wb.h"

int toy_find(const char *module)
{
    int i;
    for (i = 0; i < NTOYS; i++) {
        const char *a = TOYS[i]->module, *b = module;
        while (*a && *a == *b)
            a++, b++;
        if (*a == 0 && *b == 0)
            return i;
    }
    return -1;
}

int toy_length(toy_t *t)
{
    int n = t->c->length ? t->c->length(t) : 0;
    return n > 0 ? n : 16;
}

static void t_make(toy_t *t)
{
    uint32_t i;
    for (i = 0; i < MAX_STATE / 4; i++)
        t->state[i] = 0;
    t->np = 0;
    t->c->make(t);
}

static void t_changed(toy_t *t)
{
    if (t->c->changed)
        t->c->changed(t);
}

static void t_play_changed(toy_t *t)
{
    if (t->c->play_changed)
        t->c->play_changed(t);
}

void toy_open(toy_t *t, int id, int seed)
{
    int i, ok = 0;
    t->c = TOYS[id];
    t->id = id;
    t->seed = (uint16_t)seed;
    dice_start(&t->dice, seed);
    dice_use(&t->dice);
    t->playing = 0;
    t->pos = t->steps = t->acc = t->facc = 0;
    t->step_ms = 125;
    t->seat = 0;
    t->kicks_seen = 0;
    t_make(t);
    /* Dark.enter: start in a dark scale */
    for (i = 0; i < NDARK; i++)
        if (S.scale.value == DARK_SCALES[i])
            ok = 1;
    if (!ok && !S.band)
        S.scale.value = DARK_SCALES[0];
}

void toy_load(toy_t *t, const int16_t *vals)
{
    int i, pass;
    /* as Playbox: the values, changed() so the toy fixes up its limits, the values clamped to those
       limits, changed() again. A value outside a knob's range (a bad record) is clamped on the first
       pass too, before the toy can index a table with it; a knob whose range grows (Gate's Hits
       follows its Steps) gets its full value on the second. */
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < t->np; i++) {
            int v = vals[i];
            t->p[i].value = (int16_t)(v < t->p[i].lo ? t->p[i].lo : v > t->p[i].hi ? t->p[i].hi : v);
        }
        t_changed(t);
    }
}

void toy_restart(toy_t *t)
{
    int16_t vals[MAX_PARAMS];
    int i;
    for (i = 0; i < t->np; i++)
        vals[i] = t->p[i].value;
    dice_start(&t->dice, t->seed);
    dice_use(&t->dice);
    t_make(t);
    for (i = 0; i < t->np; i++)
        t->p[i].value = vals[i];
    t_changed(t);
    if (t->c->opened)
        t->c->opened(t);                /* as enter() drew: some toys set up state there */
    t->pos = t->steps = t->acc = t->facc = 0;
    t_play_changed(t);
}

void toy_shake(toy_t *t, int seed)
{
    dice_t aside;
    t->seed = (uint16_t)seed;
    dice_aside(&aside, seed, 1);
    dice_use(&aside);
    if (t->c->randomise)
        t->c->randomise(t);
    S.scale.value = DARK_SCALES[dice_pick(NDARK)];
    if (!S.band)                        /* in the band, its Random owns the chords */
        chords_randomise();
    toy_restart(t);
}

void toy_play(toy_t *t, int on)
{
    t->playing = on;
    if (on)
        toy_restart(t);
    t_play_changed(t);
}

void toy_scale_next(toy_t *t)
{
    int i, at = -1;
    (void)t;
    for (i = 0; i < NDARK; i++)
        if (DARK_SCALES[i] == S.scale.value)
            at = i;
    S.scale.value = DARK_SCALES[(at + 1) % NDARK];
}

void toy_turn(toy_t *t, int k, int detents)
{
    if (k < 0 || k >= t->np)
        return;
    if (param_turn(&t->p[k], detents)) {
        t_changed(t);
        t->pos %= toy_length(t);
    }
}

static void advance(toy_t *t, int ms)
{
    if (S.speed <= 0)                   /* the tape has stopped (Screw) */
        return;
    t->step_ms = 1500000 / (S.tempo.value * S.speed);   /* four steps to the beat */
    if (t->playing) {
        t->acc += ms;
        if (t->acc >= t->step_ms) {
            int n = t->acc / t->step_ms;
            t->acc -= n * t->step_ms;
            t->pos = (t->pos + n) % toy_length(t);
            if (t->c->on_step)
                t->c->on_step(t);
            t->steps += n;
        }
    }
}

void toy_tick(toy_t *t, int ms)
{
    dice_use(&t->dice);
    advance(t, ms);
    if (t->c->frame_ms && t->c->frame) {
        t->facc += ms;
        if (t->facc >= t->c->frame_ms) {
            int m = t->facc;
            t->facc = 0;
            t->c->frame(t, m < 150 ? m : 150);
        }
    }
}

void toy_note(toy_t *t, int degree, int voice, int hold)
{
    heard(t->c->role, degree, voice, t->id, t->seat, hold);
}

void toy_close(toy_t *t)
{
    if (t->c->close)
        t->c->close(t);
}

/* ---------------------------------------------------------------------------------- Dark --- */
int toy_root(toy_t *t, int ahead) { return ((chords_root(t->steps, ahead) % 7) + 7) % 7; }
int toy_now(toy_t *t) { return S.band ? S.band_steps : t->steps; }
int toy_bar(toy_t *t) { return toy_now(t) / BAR; }
void toy_kick(toy_t *t) { toy_note(t, 0, 0, 0); }

int toy_new_kick(toy_t *t)
{
    if (S.kicks != t->kicks_seen) {
        t->kicks_seen = S.kicks;
        return 1;
    }
    return 0;
}
