/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 sound engine (see wb_engine.h). Playbox's engine.cpp, written out in C: the voices,
 * the drums and the mix are the same design; the building blocks that were DaisySP's (oscillators,
 * the state-variable filter, the envelope, the drum models from Mutable Instruments' Plaits) are
 * written out here in their simplest form. Float only, fastmath.h for the maths, no libm, no double.
 *
 * Compiled as its own unit (-O2). The host builds include it too. */
#include <stdint.h>
#include "fastmath.h"
#include "wb_engine.h"
#include "wb_samples.h"

#ifdef EN_HOST
#define EN_POOL_ATTR
#else
#define EN_POOL_ATTR __attribute__((section(".pool")))
#endif

#define SR ((float)EN_SR)
#define CTL 16                      /* slow things (glide, filter moves) are updated every CTL samples */
#define TWOPI FM_TWO_PI

/* ------------------------------------------------------------------------------------ bits --- */
static uint32_t rng = 0x1234ABCDu;
static inline float frand(void)    /* -1 .. 1 */
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (float)(int32_t)rng * (1.0f / 2147483648.0f);
}
static inline float semis(float s) { return fm_exp2f(s * (1.0f / 12.0f)); }
static inline float mtof(float n) { return 440.0f * fm_exp2f((n - 69.0f) * (1.0f / 12.0f)); }
static inline float softclip(float x) { return x < -3.0f ? -1.0f : x > 3.0f ? 1.0f : x * (27.0f + x * x) / (27.0f + 9.0f * x * x); }
static inline float clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }

/* an oscillator: saw and square with a polyblep corner, triangle, sine */
enum { W_SAW, W_SQR, W_TRI, W_SIN };
typedef struct { float ph, inc, pw; uint8_t w; } osc_t;
static inline float blep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}
static inline float osc_run(osc_t *o)
{
    float t = o->ph, dt = o->inc, s;
    o->ph += dt;
    if (o->ph >= 1.0f)
        o->ph -= 1.0f;
    switch (o->w) {
    case W_SAW:
        s = 2.0f * t - 1.0f - blep(t, dt);
        break;
    case W_SQR: {
        float t2 = t + 1.0f - o->pw;
        if (t2 >= 1.0f)
            t2 -= 1.0f;
        s = (t < o->pw ? 1.0f : -1.0f) + blep(t, dt) - blep(t2, dt);
        break;
    }
    case W_TRI:
        s = t < 0.5f ? 4.0f * t - 1.0f : 3.0f - 4.0f * t;
        break;
    default:
        s = fm_sin_turns(t);
    }
    return s * 0.5f;
}
static inline void osc_freq(osc_t *o, float f) { o->inc = f * (1.0f / SR); }

/* a state-variable filter (Chamberlin) */
typedef struct { float lo, bd, hi, f, damp, res; } svf_t;
static void svf_set(svf_t *v, float fc, float res)   /* run twice a sample, so it holds up to 7 kHz */
{
    float f = 2.0f * fm_sin_pi(FM_PI * clampf(fc, 20.0f, 7000.0f) * (0.5f / SR));
    v->f = f;
    v->res = res;
    v->damp = fm_minf(2.0f * (1.0f - fm_powf(clampf(res, 0.0f, 0.98f), 0.25f)), fm_minf(2.0f, 2.0f / f - f * 0.5f));
}
static inline void svf_run(svf_t *v, float in)
{
    int k;
    for (k = 0; k < 2; k++) {
        float notch = in - v->damp * v->bd;
        v->lo = v->lo + v->f * v->bd;
        v->hi = notch - v->lo;
        v->bd = v->f * v->hi + v->bd;
    }
    v->bd = fm_flush(v->bd);
    v->lo = fm_flush(v->lo);
}

/* an envelope: A D S R in seconds and a level */
typedef struct { float lvl, atk, dec, sus, rel; uint8_t stage; } adsr_t;
enum { ST_OFF, ST_ATK, ST_DEC, ST_SUS, ST_REL };
static void adsr_set(adsr_t *e, float a, float d, float s, float r)
{
    e->atk = 1.0f / (fm_maxf(a, 0.0005f) * SR);
    e->dec = fm_expf(-4.0f / (fm_maxf(d, 0.002f) * SR));
    e->sus = s;
    e->rel = fm_expf(-4.0f / (fm_maxf(r, 0.002f) * SR));
    e->stage = ST_ATK;
}
static inline float adsr_run(adsr_t *e, int gate)
{
    if (!gate && e->stage != ST_OFF)
        e->stage = ST_REL;
    switch (e->stage) {
    case ST_ATK:
        e->lvl += e->atk;
        if (e->lvl >= 1.0f) {
            e->lvl = 1.0f;
            e->stage = ST_DEC;
        }
        break;
    case ST_DEC:
        e->lvl = e->sus + (e->lvl - e->sus) * e->dec;
        if (e->lvl - e->sus < 0.001f)
            e->stage = ST_SUS;
        break;
    case ST_SUS:
        e->lvl = e->sus;
        break;
    case ST_REL:
        e->lvl *= e->rel;
        if (e->lvl < 0.0005f) {
            e->lvl = 0.0f;
            e->stage = ST_OFF;
        }
        break;
    default:
        return 0.0f;
    }
    return e->lvl;
}
static inline int adsr_running(const adsr_t *e) { return e->stage != ST_OFF; }

/* ---------------------------------------------------------------------------------- patches --- */
enum { E_ANALOG, E_FM, E_STRING, E_ORGAN, E_FORMANT, E_BOOM, E_SAMPLE, E_NOISE };

typedef struct {
    const char *name;
    uint8_t eng;
    int8_t oct;                     /* octaves up or down from the note asked for */
    uint16_t gate;                  /* ms a note is held when the toy does not say */
    float lvl, send;                /* level, and how much goes into the echo */
    float A, D, S, R;               /* amplitude envelope */
    uint8_t w1, w2, uni;            /* analog: waves; uni: saws stacked (1..4) */
    float det, mix2, sub, noise, cut, res, env, fdec, glide, drive, spread, vib, pw, crush, bend, fatk;
    float a, b, c, d, e;            /* the other engines read these five, each in its own way */
} patch_t;
/* fm:      a ratio, b index at the start, c index it settles to, d fall time of the index
 * string:  a how hard the pluck is, b brightness, c how long it rings (0..1)
 * organ:   a..e drawbars at 1/2, 1, 2, 3 and 4 times the note
 * voice:   a, b the two formant frequencies; c breath; d how long it slides up into the note
 * boom:    a decay, b tone, c pitch punch, d life in seconds
 * sample:  a which recording, b its own pitch as a MIDI note (0 = as recorded), c 1 to loop
 * noise:   a 0 low-pass, 1 high-pass, 2 band-pass; b filter start Hz; c filter end Hz (swept over the hold) */

#define ENV(a_, d_, s_, r_) .A = a_, .D = d_, .S = s_, .R = r_
#define P0 .lvl = 0.8f, .A = 0.004f, .D = 0.25f, .S = 0.7f, .R = 0.25f, .w1 = W_SAW, .w2 = W_SAW, .uni = 1, .cut = 6000.0f, .res = 0.1f, .fdec = 0.2f, .pw = 0.5f, .gate = 250
static const patch_t PATCH[I_COUNT] = {
    {"Sub", E_ANALOG, -1, 300, 0.9f, 0, 0.004f, 0.4f, 0.9f, 0.15f, W_SIN, W_SIN, 1, 0.08f, 0.5f, 0, 0, 500.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"808 Boom", E_BOOM, -1, 250, 5.0f, 0, 0.004f, 0.25f, 0.7f, 0.25f, W_SAW, W_SAW, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.75f, 0.25f, 0.35f, 2.2f, 0},
    {"808 Slide", E_BOOM, -1, 250, 5.0f, 0, 0.004f, 0.25f, 0.7f, 0.25f, W_SAW, W_SAW, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0.12f, 0, 0, 0, 0.5f, 0, 0, 0, 0.7f, 0.3f, 0.3f, 2.0f, 0},
    {"Acid", E_ANALOG, -1, 110, 0.8f, 0, 0.004f, 0.15f, 0.6f, 0.06f, W_SAW, W_SAW, 1, 0, 0, 0, 0, 300.0f, 0.82f, 3.5f, 0.14f, 0.05f, 0.3f, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Reese", E_ANALOG, -1, 500, 0.8f, 0, 0.02f, 0.25f, 0.9f, 0.2f, W_SAW, W_SAW, 1, 0.22f, 1.0f, 0, 0, 420.0f, 0.25f, 0, 0.2f, 0, 0.2f, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Saw Bass", E_ANALOG, -1, 120, 0.8f, 0, 0.004f, 0.2f, 0.5f, 0.08f, W_SAW, W_SAW, 1, 0, 0, 0.5f, 0, 900.0f, 0.2f, 2.0f, 0.12f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"FM Bass", E_FM, -1, 160, 0.8f, 0, 0.004f, 0.3f, 0.4f, 0.1f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 1.0f, 3.5f, 0.6f, 0.09f, 0},
    {"Pluck Bass", E_STRING, -1, 250, 1.1f, 0, 0.004f, 0.25f, 0.7f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.4f, 0.35f, 0.55f, 0, 0},
    {"Soft Keys", E_FM, 0, 260, 0.7f, 0.15f, 0.006f, 0.6f, 0.35f, 0.35f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 1.0f, 1.4f, 0.3f, 0.25f, 0},
    {"Electric Piano", E_FM, 0, 300, 0.7f, 0.12f, 0.004f, 0.9f, 0.25f, 0.4f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 14.0f, 1.1f, 0.08f, 0.12f, 0},
    {"Dull Piano", E_STRING, 0, 250, 1.2f, 0.25f, 0.004f, 0.25f, 0.7f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0.12f, 0.5f, 0, 0, 0, 0.15f, 0.2f, 0.8f, 0, 0},
    {"Toy Piano", E_FM, 1, 40, 0.6f, 0, 0.001f, 0.5f, 0.0f, 0.35f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 4.0f, 2.4f, 0.3f, 0.07f, 0},
    {"Clav", E_ANALOG, 0, 140, 0.8f, 0, 0.004f, 0.25f, 0.2f, 0.08f, W_SQR, W_SAW, 1, 0, 0, 0, 0, 2400.0f, 0.3f, 1.5f, 0.1f, 0, 0, 0, 0, 0.22f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Music Box", E_FM, 2, 40, 0.55f, 0.2f, 0.001f, 1.1f, 0.0f, 0.8f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 5.0f, 1.3f, 0.15f, 0.2f, 0},
    {"Soul Organ", E_ORGAN, 0, 260, 0.6f, 0, 0.004f, 0.25f, 1.0f, 0.06f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.7f, 1.0f, 0.6f, 0.0f, 0.15f},
    {"Church Organ", E_ORGAN, 0, 900, 0.5f, 0.3f, 0.08f, 0.25f, 1.0f, 0.5f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.8f, 1.0f, 0.8f, 0.5f, 0.6f},
    {"Choir Aah", E_FORMANT, 0, 900, 0.7f, 0.3f, 0.15f, 0.25f, 0.9f, 0.6f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0.15f, 0.5f, 0, 0, 0, 800.0f, 1200.0f, 0.1f, 0.0f, 0},
    {"Choir Ooh", E_FORMANT, 0, 900, 0.7f, 0.3f, 0.2f, 0.25f, 0.9f, 0.7f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0.12f, 0.5f, 0, 0, 0, 380.0f, 820.0f, 0.25f, 0.0f, 0},
    {"Warm Pad", E_ANALOG, 0, 900, 0.55f, 0.2f, 0.25f, 0.6f, 0.8f, 0.8f, W_SAW, W_SAW, 1, 0.09f, 1.0f, 0, 0, 1100.0f, 0.15f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Dark Strings", E_ANALOG, 0, 900, 0.5f, 0.25f, 0.3f, 0.25f, 0.9f, 0.9f, W_SAW, W_SAW, 4, 0, 0, 0, 0, 700.0f, 0.1f, 0, 0.2f, 0, 0, 0.14f, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Glass Pad", E_FM, 1, 900, 0.45f, 0.35f, 0.3f, 1.0f, 0.7f, 1.0f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 3.01f, 1.2f, 0.5f, 0.8f, 0},
    {"Saw Pad", E_ANALOG, 0, 700, 0.5f, 0.2f, 0.08f, 0.25f, 0.9f, 0.5f, W_SAW, W_SAW, 4, 0, 0, 0, 0, 3200.0f, 0.1f, 0, 0.2f, 0, 0, 0.16f, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Coven Saws", E_ANALOG, 0, 1800, 0.5f, 0.25f, 0.05f, 0.25f, 1.0f, 0.6f, W_SAW, W_SAW, 4, 0, 0, 0, 0, 450.0f, 0.2f, 2.6f, 0.2f, 0, 0, 0.3f, 0, 0.5f, 0, 0, 1.6f, 0, 0, 0, 0, 0},
    {"Drone", E_ANALOG, -1, 4000, 0.6f, 0, 0.6f, 0.25f, 1.0f, 1.5f, W_SAW, W_SAW, 1, 0.05f, 1.0f, 0.5f, 0, 350.0f, 0.3f, 1.5f, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 3.0f, 0, 0, 0, 0, 0},
    {"Saw Lead", E_ANALOG, 0, 180, 0.6f, 0.2f, 0.004f, 0.3f, 0.7f, 0.15f, W_SAW, W_SAW, 1, 0.1f, 0.8f, 0, 0, 3500.0f, 0.15f, 0.8f, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Square Lead", E_ANALOG, 0, 180, 0.55f, 0.2f, 0.004f, 0.25f, 0.8f, 0.12f, W_SQR, W_SAW, 1, 0, 0, 0, 0, 3000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0.1f, 0.4f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Chip Lead", E_ANALOG, 0, 110, 0.5f, 0, 0.004f, 0.25f, 1.0f, 0.02f, W_SQR, W_SAW, 1, 0, 0, 0, 0, 9000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.25f, 0.55f, 0, 0, 0, 0, 0, 0, 0},
    {"Glide Lead", E_ANALOG, 0, 700, 0.6f, 0.3f, 0.05f, 0.25f, 0.9f, 0.4f, W_TRI, W_SIN, 1, 0, 0.6f, 0, 0, 2500.0f, 0.1f, 0, 0.2f, 0.35f, 0, 0, 0.25f, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Screamer", E_ANALOG, 1, 90, 0.55f, 0.15f, 0.004f, 0.25f, 0.9f, 0.05f, W_SAW, W_SQR, 1, 7.02f, 0.7f, 0, 0.15f, 2600.0f, 0.55f, 1.5f, 0.2f, 0, 0.9f, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Whistle", E_ANALOG, 1, 320, 0.5f, 0.35f, 0.04f, 0.25f, 0.9f, 0.3f, W_SIN, W_SIN, 1, 0, 0, 0, 0.05f, 5000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0.18f, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Glass Bell", E_FM, 1, 60, 0.6f, 0.4f, 0.001f, 1.6f, 0.0f, 1.2f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 3.52f, 2.2f, 0.3f, 0.6f, 0},
    {"Kalimba", E_FM, 1, 40, 0.7f, 0.15f, 0.001f, 0.7f, 0.0f, 0.5f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 3.0f, 1.6f, 0.05f, 0.05f, 0},
    {"Marimba", E_FM, 0, 40, 0.8f, 0.1f, 0.001f, 0.45f, 0.0f, 0.3f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 3.0f, 2.0f, 0.0f, 0.035f, 0},
    {"Harp", E_STRING, 0, 250, 1.1f, 0.2f, 0.004f, 0.25f, 0.7f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.3f, 0.6f, 0.85f, 0, 0},
    {"Guitar", E_STRING, 0, 250, 1.1f, 0.1f, 0.004f, 0.25f, 0.7f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.7f, 0.5f, 0.7f, 0, 0},
    {"Trap Bell", E_FM, 1, 50, 0.55f, 0.25f, 0.001f, 0.5f, 0.0f, 0.4f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 5.0f, 1.8f, 0.1f, 0.15f, 0},
    {"Rave Stab", E_ANALOG, 0, 120, 0.5f, 0.2f, 0.004f, 0.2f, 0.5f, 0.1f, W_SAW, W_SAW, 3, 0, 0, 0, 0, 1800.0f, 0.35f, 2.0f, 0.12f, 0, 0.3f, 0.2f, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Minor Stab", E_ANALOG, 0, 140, 0.55f, 0.15f, 0.004f, 0.25f, 0.3f, 0.12f, W_SAW, W_SAW, 1, 0.08f, 0.8f, 0, 0, 1400.0f, 0.2f, 1.6f, 0.15f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0},
    {"Ghost Voice", E_FORMANT, 1, 450, 0.6f, 0.45f, 0.06f, 0.25f, 0.9f, 0.5f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0.12f, 0, 0, 0.2f, 0.5f, 0, 0, 0, 650.0f, 1080.0f, 0.3f, 0.08f, 0},
    {"Screwed Voice", E_FORMANT, -1, 1600, 0.75f, 0.3f, 0.2f, 0.25f, 0.9f, 0.9f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0.3f, 0, 0, 0.1f, 0.5f, 0, 0, 0, 500.0f, 800.0f, 0.15f, 0.35f, 0},
    {"Crackle", E_SAMPLE, 0, 2400, 5.0f, 0, 0.05f, 0.25f, 1.0f, 0.3f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_CRACKLE, 0, 1, 0, 0},
    {"Rain", E_NOISE, 0, 2400, 0.25f, 0, 0.3f, 0.25f, 1.0f, 0.6f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 1, 5000.0f, 5000.0f, 0, 0},
    {"Rumble", E_NOISE, 0, 2400, 0.9f, 0, 0.4f, 0.25f, 1.0f, 0.8f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 90.0f, 90.0f, 0, 0},
    {"City Hum", E_SAMPLE, 0, 2400, 2.5f, 0, 0.3f, 0.25f, 1.0f, 0.6f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_HUM, 0, 1, 0, 0},
    {"Riser", E_NOISE, 0, 2000, 0.45f, 0.2f, 1.0f, 0.25f, 1.0f, 0.05f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.5f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 2, 250.0f, 7000.0f, 0, 0},
    {"Impact", E_SAMPLE, 0, 1000, 1.0f, 0.3f, 0.004f, 0.25f, 1.0f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_IMPACT, 0, 0, 0, 0},
    {"Woosh", E_SAMPLE, 0, 2500, 0.8f, 0.2f, 0.004f, 0.25f, 1.0f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_WOOSH, 0, 0, 0, 0},
    {"Door", E_SAMPLE, 0, 1200, 0.8f, 0.3f, 0.004f, 0.25f, 1.0f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_DOOR, 0, 0, 0, 0},
    {"Crow", E_SAMPLE, 0, 500, 0.6f, 0.5f, 0.004f, 0.25f, 1.0f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_CROW, 0, 0, 0, 0},
    {"Skip", E_SAMPLE, 0, 270, 0.8f, 0, 0.004f, 0.25f, 1.0f, 0.25f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_SKIP, 0, 0, 0, 0},
    {"Vox Hit", E_SAMPLE, 0, 200, 0.9f, 0.25f, 0.004f, 0.25f, 1.0f, 0.08f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, S_VOX_HIT, 36, 0, 0, 0},
    /* witchbox1's own */
    {"Toll Bell", E_FM, -1, 80, 0.7f, 0.5f, 0.001f, 3.5f, 0.0f, 3.0f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 2.76f, 1.6f, 0.2f, 1.2f, 0},
    {"Cold Keys", E_FM, 0, 400, 0.6f, 0.35f, 0.01f, 1.2f, 0.4f, 0.9f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0.08f, 0.5f, 0, 0, 0, 2.0f, 1.0f, 0.25f, 0.5f, 0},
    {"Thunder", E_NOISE, 0, 1800, 1.1f, 0.3f, 0.02f, 0.25f, 1.0f, 1.2f, 0, 0, 1, 0, 0, 0, 0, 6000.0f, 0.3f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 400.0f, 60.0f, 0, 0},
};

/* one row of settings for each kit; the drum code below reads them */
typedef struct {
    float kick_f, kick_dec, kick_tone; uint8_t kick_synth;     /* synth = the punchy model, else the boomy one */
    float sn_f, sn_dec, sn_snap, sn_tone;
    float hat_f, hat_dec, hat_tone, hat_noise;
    float tom_f, tom_dec;
    float clap_tail;                /* seconds */
    float drive, crush;
} kit_t;
static const kit_t KIT[K_COUNT] = {
    /* 808 */      {48, 0.55f, 0.20f, 0, 190, 0.30f, 0.60f, 0.5f, 3300, 0.22f, 0.55f, 0.80f, 95, 0.45f, 0.25f, 0.0f, 0.0f},
    /* 909 */      {55, 0.45f, 0.55f, 1, 220, 0.25f, 0.75f, 0.7f, 4200, 0.18f, 0.75f, 0.90f, 110, 0.35f, 0.20f, 0.2f, 0.0f},
    /* Acoustic */ {62, 0.30f, 0.45f, 1, 200, 0.35f, 0.85f, 0.6f, 3600, 0.25f, 0.60f, 1.00f, 120, 0.40f, 0.15f, 0.0f, 0.0f},
    /* Lo-fi */    {52, 0.45f, 0.15f, 0, 175, 0.28f, 0.50f, 0.3f, 2600, 0.18f, 0.30f, 0.85f, 90, 0.40f, 0.20f, 0.3f, 0.5f},
    /* Witch */    {40, 0.85f, 0.15f, 0, 160, 0.50f, 0.70f, 0.4f, 3000, 0.25f, 0.45f, 0.90f, 70, 0.75f, 0.90f, 0.15f, 0.0f},
    /* Garage */   {58, 0.35f, 0.50f, 1, 240, 0.18f, 0.55f, 0.8f, 4600, 0.14f, 0.80f, 0.90f, 115, 0.30f, 0.15f, 0.1f, 0.0f},
    /* Chip */     {70, 0.25f, 0.80f, 1, 280, 0.15f, 0.90f, 0.9f, 5200, 0.10f, 0.90f, 0.60f, 140, 0.25f, 0.10f, 0.0f, 0.75f},
    /* Tin */      {80, 0.20f, 0.70f, 1, 330, 0.12f, 0.40f, 0.9f, 5600, 0.12f, 0.95f, 0.50f, 180, 0.20f, 0.10f, 0.4f, 0.3f},
};

/* ----------------------------------------------------------------------------------- shared --- */
static float g_level[EN_SEATS], g_tone[EN_SEATS], g_send[EN_SEATS];
static float g_master = 0.8f, g_drop = 1.0f;
static float g_echo = 0.5f, g_feedback = 0.38f, g_drive = 0.0f, g_crush = 0.0f;
static uint32_t g_clock;            /* counts notes, to find the oldest voice */
static uint32_t g_tick;             /* counts samples */

#define ECHO_MAX (EN_SR * 2 / 5)    /* 0.4 s */
static int16_t echo_buf[ECHO_MAX] EN_POOL_ATTR;
static int echo_len = EN_SR / 4, echo_at;

typedef struct {                    /* what every voice has */
    uint8_t on, seat;
    const patch_t *p;
    float vel, freq;
    int32_t hold;                   /* samples until the note is let go */
    uint32_t born;
} head_t;

static void begin(head_t *h, const patch_t *p, int seat, float freq, float vel, int hold_ms)
{
    h->on = 1;
    h->seat = (uint8_t)seat;
    h->p = p;
    h->vel = vel;
    h->freq = freq;
    h->hold = (int32_t)((float)(hold_ms ? hold_ms : p->gate) * (SR / 1000.0f));
    h->born = ++g_clock;
}

static inline int gate_run(head_t *h, adsr_t *e, float *a)
{
    *a = adsr_run(e, h->hold > 0);
    if (h->hold > 0)
        h->hold--;
    else if (!adsr_running(e))
        return 0;
    return 1;
}

/* ----------------------------------------------------------------------------------- analog --- */
typedef struct {
    head_t h;
    osc_t o[4];
    svf_t f;
    adsr_t amp;
    float now, sub_ph, fenv, fstep, lfo, held;
    int crush_n;
} analog_t;

static void analog_tune(analog_t *v)
{
    const patch_t *p = v->h.p;
    float fr, cut;
    if (p->glide > 0.0f)
        v->now += (v->h.freq - v->now) * (CTL / (p->glide * SR * 0.35f + CTL));
    v->lfo += CTL * 5.5f / SR;
    if (v->lfo > 1.0f)
        v->lfo -= 1.0f;
    fr = v->now * g_drop;
    if (p->vib > 0.0f)
        fr *= 1.0f + p->vib * 0.06f * fm_sin_turns(v->lfo);
    if (p->bend != 0.0f)
        fr *= semis(p->bend * (1.0f - v->fenv));
    if (v->fstep < 0.0f)
        v->fenv *= 1.0f - CTL / (p->fdec * SR);
    else if (v->fenv < 1.0f)
        v->fenv += v->fstep;
    osc_freq(&v->o[0], fr);
    if (p->uni > 1) {               /* a stack of saws, spread either side */
        float s = p->spread * 0.06f;
        osc_freq(&v->o[1], fr * (1.0f + s));
        osc_freq(&v->o[2], fr * (1.0f - s * 0.62f));
        osc_freq(&v->o[3], fr * (1.0f + s * 0.37f) * 0.5f);
    } else {
        osc_freq(&v->o[1], fr * semis(p->det));
    }
    cut = p->cut * g_tone[v->h.seat] * fm_exp2f(p->env * v->fenv);
    svf_set(&v->f, clampf(cut, 30.0f, 7000.0f), p->res);
}

static void analog_start(analog_t *v)
{
    const patch_t *p = v->h.p;
    int i;
    if (p->glide <= 0.0f || v->now <= 0.0f)
        v->now = v->h.freq;
    v->o[0].w = p->w1;
    v->o[1].w = p->w2;
    v->o[2].w = v->o[3].w = p->w1;
    for (i = 0; i < 4; i++)
        v->o[i].pw = p->pw;
    v->f.lo = v->f.bd = v->f.hi = 0.0f;
    adsr_set(&v->amp, p->A, p->D, p->S, p->R);
    if (p->fatk > 0.0f) {           /* the filter opens */
        v->fenv = 0.0f;
        v->fstep = CTL / (p->fatk * SR);
    } else {                        /* the filter closes */
        v->fenv = 1.0f;
        v->fstep = -1.0f;
    }
    analog_tune(v);
}

static float analog_run(analog_t *v)
{
    const patch_t *p = v->h.p;
    float s = osc_run(&v->o[0]), a;
    if (p->uni > 1) {
        int i;
        for (i = 1; i < p->uni; i++)
            s += osc_run(&v->o[i]);
        s *= 0.6f;
    } else if (p->mix2 > 0.0f) {
        s += p->mix2 * osc_run(&v->o[1]);
    }
    if (p->sub > 0.0f) {
        v->sub_ph += v->now * g_drop * 0.5f / SR;
        if (v->sub_ph > 1.0f)
            v->sub_ph -= 1.0f;
        s += p->sub * fm_sin_turns(v->sub_ph);
    }
    if (p->noise > 0.0f)
        s += p->noise * frand();
    if (p->drive > 0.0f)
        s = softclip(s * (1.0f + p->drive * 6.0f)) * (1.0f - p->drive * 0.35f);
    svf_run(&v->f, s);
    s = v->f.lo;
    if (p->crush > 0.0f) {          /* hold each sample for a few, and round it off */
        int n = 1 + (int)(p->crush * 10.0f);
        if (++v->crush_n >= n) {
            float q = 4.0f + (1.0f - p->crush) * 28.0f;
            v->crush_n = 0;
            v->held = fm_floorf(s * q + 0.5f) / q;
        }
        s = v->held;
    }
    if (!gate_run(&v->h, &v->amp, &a)) {
        v->h.on = 0;
        v->now = v->h.freq;
    }
    return s * a;
}

/* --------------------------------------------------------------------------------------- fm --- */
typedef struct { head_t h; adsr_t amp; float car, mod, ienv, inc, idx, ratio; } fm_t;

static void fm_tune(fm_t *v)
{
    const patch_t *p = v->h.p;
    v->ienv *= 1.0f - CTL / (p->d * SR);
    v->inc = v->h.freq * g_drop / SR;
    v->idx = (p->c + (p->b - p->c) * v->ienv) * (0.4f + 0.6f * v->h.vel) * g_tone[v->h.seat];
}

static void fm_start(fm_t *v)
{
    const patch_t *p = v->h.p;
    v->ratio = p->a;
    v->ienv = 1.0f;
    v->car = v->mod = 0.0f;
    adsr_set(&v->amp, p->A, p->D, p->S, p->R);
    fm_tune(v);
}

static float fm_run(fm_t *v)
{
    float a, m, s;
    v->mod += v->inc * v->ratio;
    if (v->mod >= 1.0f)
        v->mod -= 1.0f;
    v->car += v->inc;
    if (v->car >= 1.0f)
        v->car -= 1.0f;
    m = fm_sin_turns(v->mod) * v->idx * 0.5f;      /* phase modulation, in turns */
    s = fm_sin_turns(v->car + m - fm_floorf(v->car + m));
    if (!gate_run(&v->h, &v->amp, &a))
        v->h.on = 0;
    return s * a * 0.8f;
}

/* ----------------------------------------------------------------------------------- string --- */
/* A plucked string, the classic way: a burst of noise goes round a delay as long as one wave of the
 * note, and is smoothed a little each time round. */
#define STRING_MAX 1024
typedef struct {
    head_t h;
    float line[STRING_MAX];
    float len, lp, keep, bright, fade;
    int at, n;
    int32_t life;
} pluck_t;

static void pluck_start(pluck_t *v)
{
    const patch_t *p = v->h.p;
    float dull = 0.0f, sharp = 0.25f + 0.75f * p->a * (0.5f + 0.5f * v->h.vel), secs;
    int i;
    v->len = clampf(SR / (v->h.freq * g_drop) - 0.5f, 8.0f, STRING_MAX - 2.0f);
    v->n = (int)v->len + 2;
    for (i = 0; i < v->n; i++) {    /* the pluck: noise, softened for gentle sounds */
        dull += (frand() - dull) * sharp;
        v->line[i] = dull;
    }
    v->at = 0;
    v->lp = 0.0f;
    v->bright = clampf(0.25f + 0.7f * p->b * g_tone[v->h.seat], 0.05f, 0.98f);
    secs = 0.3f + p->c * p->c * 6.0f;
    v->keep = fm_exp2f(-10.0f * (v->len / SR) / secs);
    v->life = (int32_t)(secs * 1.2f * SR);
    v->fade = 1.0f;
}

static void pluck_tune(pluck_t *v)
{
    if (v->h.p->vib > 0.0f)         /* a slow wobble, like worn tape */
        v->len = clampf(SR / (v->h.freq * g_drop * (1.0f + v->h.p->vib * 0.012f * fm_sin_turns((float)(g_tick % 31500u) * (0.7f / SR)))) - 0.5f,
                        8.0f, (float)(v->n - 2));
}

static float pluck_run(pluck_t *v)
{
    float rd = (float)v->at - v->len, fr, s;
    int i, j;
    if (rd < 0.0f)
        rd += (float)v->n;
    i = (int)rd;
    fr = rd - (float)i;
    j = i + 1 < v->n ? i + 1 : 0;
    s = v->line[i] * (1.0f - fr) + v->line[j] * fr;
    v->lp += (s - v->lp) * v->bright;
    v->line[v->at] = v->lp * v->keep;
    if (++v->at >= v->n)
        v->at = 0;
    if (--v->life < 0) {
        v->fade -= 1.0f / (0.03f * SR);
        if (v->fade <= 0.0f) {
            v->h.on = 0;
            return 0.0f;
        }
    }
    return s * v->fade;
}

/* ------------------------------------------------------------------------------------ organ --- */
typedef struct { head_t h; adsr_t amp; float ph[5]; } organ_t;
static void organ_start(organ_t *v)
{
    const patch_t *p = v->h.p;
    adsr_set(&v->amp, p->A, p->D, p->S, p->R);
}
static float organ_run(organ_t *v)
{
    static const float mult[5] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f};
    const float *bar = &v->h.p->a;
    float inc = v->h.freq * g_drop / SR, s = 0.0f, a;
    int i;
    for (i = 0; i < 5; i++)
        if (bar[i] > 0.0f) {
            v->ph[i] += inc * mult[i];
            if (v->ph[i] > 1.0f)
                v->ph[i] -= 1.0f;
            s += bar[i] * fm_sin_turns(v->ph[i]);
        }
    if (!gate_run(&v->h, &v->amp, &a))
        v->h.on = 0;
    return s * a * 0.35f;
}

/* ------------------------------------------------------------------------------------ voice --- */
/* Two formants: a sine at the formant's frequency, restarted on every pulse of the carrier and
 * fading over the carrier's period, as a vocal cord's pulse rings the mouth. */
typedef struct { float car, ph, inc, finc; } formant_t;
static inline float formant_run(formant_t *f)
{
    float w;
    f->car += f->inc;
    if (f->car >= 1.0f) {
        f->car -= 1.0f;
        f->ph = 0.0f;
    }
    f->ph += f->finc;
    if (f->ph >= 1.0f)
        f->ph -= 1.0f;
    w = 1.0f - f->car;
    return fm_sin_turns(f->ph) * w * w;
}
typedef struct { head_t h; formant_t f1, f2; svf_t air; adsr_t amp; float now, lfo; } vox_t;

static void vox_tune(vox_t *v)
{
    const patch_t *p = v->h.p;
    float t = p->d > 0.0f ? p->d : p->glide, fr;
    if (t > 0.0f)
        v->now += (v->h.freq - v->now) * (CTL / (t * SR * 0.35f + CTL));
    v->lfo += CTL * 5.0f / SR;
    if (v->lfo > 1.0f)
        v->lfo -= 1.0f;
    fr = v->now * g_drop * (1.0f + p->vib * 0.05f * fm_sin_turns(v->lfo));
    v->f1.inc = v->f2.inc = fr / SR;
}

static void vox_start(vox_t *v)
{
    const patch_t *p = v->h.p;
    if (p->d > 0.0f)
        v->now = v->h.freq * 0.84f;     /* slides up into the note from three semitones under */
    else if (p->glide <= 0.0f || v->now <= 0.0f)
        v->now = v->h.freq;
    v->f1.finc = p->a / SR;
    v->f2.finc = p->b / SR;
    v->f1.ph = v->f2.ph = 0.0f;
    svf_set(&v->air, 2800.0f, 0.4f);
    adsr_set(&v->amp, p->A, p->D, p->S, p->R);
    vox_tune(v);
}

static float vox_run(vox_t *v)
{
    const patch_t *p = v->h.p;
    float s = formant_run(&v->f1) + 0.55f * formant_run(&v->f2), a;
    if (p->c > 0.0f) {
        svf_run(&v->air, frand());
        s = s * (1.0f - p->c * 0.5f) + v->air.bd * p->c * 1.5f;
    }
    if (!gate_run(&v->h, &v->amp, &a)) {
        v->h.on = 0;
        v->now = v->h.freq;
    }
    return s * a * 0.6f;
}

/* ------------------------------------------------------------------------------- bass drum --- */
/* A sine with a pitch punch at the start, a long fall, some self-FM for growl and a tone control
 * (after Plaits' analog bass drum, in outline). */
typedef struct { float ph, f, decay, tone, punch, selffm, acc, env, penv, pdec, last; } bd_t;
static void bd_trig(bd_t *d)
{
    d->env = d->acc;
    d->penv = 1.0f;
    d->ph = 0.0f;
}
static inline float bd_run(bd_t *d)
{
    float f = d->f * (1.0f + d->punch * 8.0f * d->penv) * (1.0f + d->selffm * 0.3f * d->last);
    float s;
    d->ph += f / SR;
    if (d->ph >= 1.0f)
        d->ph -= 1.0f;
    s = fm_sin_turns(d->ph);
    s = s * (1.0f - d->tone) + softclip(s * (1.0f + d->tone * 3.0f)) * d->tone;
    d->last = s;
    d->penv *= d->pdec;
    d->env *= d->decay;
    return s * d->env;
}
static void bd_set(bd_t *d, float freq, float decay, float tone, float punch, float selffm, float acc)
{
    d->f = freq;
    d->decay = fm_expf(-6.9f / ((0.1f + decay * 1.2f) * SR));
    d->pdec = fm_expf(-6.9f / (0.04f * SR));
    d->tone = tone;
    d->punch = punch;
    d->selffm = selffm;
    d->acc = acc;
}

/* a snare: two pitched modes and a burst of filtered noise */
typedef struct { float f, ph1, ph2, env, nenv, dec, ndec, snap, tone; svf_t nf; } sn_t;
static void sn_set(sn_t *d, float freq, float decay, float snappy, float tone, float acc)
{
    d->f = freq;
    d->dec = fm_expf(-6.9f / ((0.05f + decay * 0.5f) * SR));
    d->ndec = fm_expf(-6.9f / ((0.03f + decay * 0.4f) * SR));
    d->snap = snappy;
    d->tone = tone;
    d->env = d->nenv = acc;
    svf_set(&d->nf, 2000.0f + tone * 4000.0f, 0.3f);
}
static inline float sn_run(sn_t *d)
{
    float s;
    d->ph1 += d->f / SR;
    d->ph2 += d->f * 1.63f / SR;
    if (d->ph1 >= 1.0f)
        d->ph1 -= 1.0f;
    if (d->ph2 >= 1.0f)
        d->ph2 -= 1.0f;
    s = (fm_sin_turns(d->ph1) + 0.6f * fm_sin_turns(d->ph2)) * d->env * (1.0f - d->snap * 0.5f);
    svf_run(&d->nf, frand());
    s += d->nf.hi * d->nenv * d->snap * 1.2f;
    d->env *= d->dec;
    d->nenv *= d->ndec;
    return s;
}

/* a hat: six square waves, a band pass, a fast fall (the 808's way) */
typedef struct { float ph[6], f, env, dec, noise; svf_t bp; } hat_t;
static void hat_set(hat_t *d, float freq, float decay, float tone, float noise, float acc)
{
    d->f = freq;
    d->dec = fm_expf(-6.9f / ((0.02f + decay * 0.6f) * SR));
    d->noise = noise;
    d->env = acc;
    svf_set(&d->bp, clampf(freq * (0.8f + tone), 1500.0f, 8500.0f), 0.5f);
}
static inline float hat_run(hat_t *d)
{
    static const float r[6] = {1.0f, 1.342f, 1.2312f, 1.6532f, 1.9523f, 2.1523f};
    float s = 0.0f;
    int i;
    for (i = 0; i < 6; i++) {
        d->ph[i] += d->f * r[i] / SR;
        if (d->ph[i] >= 1.0f)
            d->ph[i] -= 1.0f;
        s += d->ph[i] < 0.5f ? 0.16f : -0.16f;
    }
    s = s * (1.0f - d->noise * 0.5f) + frand() * d->noise * 0.5f;
    svf_run(&d->bp, s);
    d->env *= d->dec;
    return d->bp.hi * d->env * 2.0f;
}

/* ------------------------------------------------------------------------------------- boom --- */
/* The drum model sounds three semitones above the pitch it is given, so it is asked for lower. */
#define BOOM_TUNE 0.8409f
typedef struct { head_t h; bd_t bd; float now, fade; int32_t life; } boom_t;
static void boom_start(boom_t *v)
{
    const patch_t *p = v->h.p;
    if (p->glide <= 0.0f || v->now <= 0.0f || v->life <= 0)
        v->now = v->h.freq;
    bd_set(&v->bd, v->now * g_drop * BOOM_TUNE, p->a, clampf(p->b * g_tone[v->h.seat], 0.0f, 1.0f), p->c, 0.2f,
           0.4f + 0.6f * v->h.vel);
    bd_trig(&v->bd);
    v->life = (int32_t)(p->d * SR);
    v->fade = 1.0f;
}
static void boom_tune(boom_t *v)
{
    if (v->h.p->glide > 0.0f)
        v->now += (v->h.freq - v->now) * (CTL / (v->h.p->glide * SR * 0.35f + CTL));
    v->bd.f = v->now * g_drop * BOOM_TUNE;
}
static float boom_run(boom_t *v)
{
    float s = bd_run(&v->bd);
    if (--v->life < 0) {
        v->fade -= 1.0f / (0.03f * SR);
        if (v->fade <= 0.0f) {
            v->h.on = 0;
            return 0.0f;
        }
    }
    return s * v->fade;
}

/* ----------------------------------------------------------------------------------- sample --- */
typedef struct { head_t h; adsr_t amp; const wb_sample_t *smp; float pos, inc; } tape_t;
static inline float smp_at(const wb_sample_t *s, uint32_t i) { return (float)(s->data[i] << s->shift[i >> 8]) * (1.0f / 32768.0f); }
static void tape_tune(tape_t *v)
{
    const patch_t *p = v->h.p;
    float r = (float)v->smp->rate / SR * g_drop;
    if (p->b > 0.0f)                /* played at the pitch asked for */
        r *= v->h.freq / mtof(p->b);
    else
        r *= v->h.freq;             /* freq carries a plain speed for unpitched recordings */
    v->inc = r;
}
static void tape_start(tape_t *v)
{
    const patch_t *p = v->h.p;
    v->smp = &wb_samples[(int)p->a];
    v->pos = 0.0f;
    adsr_set(&v->amp, p->A, p->D, p->S, p->R);
    tape_tune(v);
}
static float tape_run(tape_t *v)
{
    uint32_t i = (uint32_t)v->pos;
    float fr, s, a;
    if (i + 1 >= v->smp->len) {
        if (v->h.p->c > 0.0f) {
            v->pos -= (float)(v->smp->len - 1);
            i = (uint32_t)v->pos;
        } else {
            v->h.on = 0;
            return 0.0f;
        }
    }
    fr = v->pos - (float)i;
    s = smp_at(v->smp, i) * (1.0f - fr) + smp_at(v->smp, i + 1) * fr;
    v->pos += v->inc;
    if (!gate_run(&v->h, &v->amp, &a))
        v->h.on = 0;
    return s * a;
}

/* ------------------------------------------------------------------------------------ noise --- */
typedef struct { head_t h; svf_t f; adsr_t amp; float cut, mul; } hiss_t;
static void hiss_start(hiss_t *v)
{
    const patch_t *p = v->h.p;
    v->cut = p->b;
    v->mul = fm_powf(p->c / p->b, (float)CTL / (float)(v->h.hold > 0 ? v->h.hold : 1));
    svf_set(&v->f, v->cut, p->res);
    adsr_set(&v->amp, p->A < 0.9f ? p->A : (float)v->h.hold / SR, p->D, p->S, p->R);  /* a riser climbs for as long as it is held */
}
static void hiss_tune(hiss_t *v)
{
    if (v->mul != 1.0f && v->h.hold > 0) {
        v->cut *= v->mul;
        svf_set(&v->f, clampf(v->cut * g_drop, 30.0f, 7000.0f), v->h.p->res);
    }
}
static float hiss_run(hiss_t *v)
{
    int mode = (int)v->h.p->a;
    float s, a;
    svf_run(&v->f, frand());
    s = mode == 0 ? v->f.lo * 2.0f : mode == 1 ? v->f.hi : v->f.bd;
    if (!gate_run(&v->h, &v->amp, &a))
        v->h.on = 0;
    return s * a;
}

/* ------------------------------------------------------------------------------------ pools --- */
#define N_ANALOG 6
#define N_FM 4
#define N_STRING 3
#define N_ORGAN 3
#define N_VOX 3
#define N_BOOM 2
#define N_TAPE 4
#define N_HISS 3
static analog_t g_analog[N_ANALOG] EN_POOL_ATTR;
static fm_t g_fm[N_FM] EN_POOL_ATTR;
static pluck_t g_string[N_STRING] EN_POOL_ATTR;
static organ_t g_organ[N_ORGAN] EN_POOL_ATTR;
static vox_t g_vox[N_VOX] EN_POOL_ATTR;
static boom_t g_boom[N_BOOM] EN_POOL_ATTR;
static tape_t g_tape[N_TAPE] EN_POOL_ATTR;
static hiss_t g_hiss[N_HISS] EN_POOL_ATTR;

/* the voice to take: a free one, else the oldest. All voices start with a head_t. */
static void *take(void *pool, int n, int size, int mono)
{
    char *base = (char *)pool, *best = base;
    int i;
    if (mono)
        return base;
    for (i = 0; i < n; i++) {
        head_t *h = (head_t *)(base + i * size);
        if (!h->on)
            return h;
        if (h->born < ((head_t *)best)->born)
            best = (char *)h;
    }
    return best;
}

/* ------------------------------------------------------------------------------------ drums --- */
typedef struct { uint8_t seat; float lvl, drive, crush, held; int32_t left; int n; } hit_t;
static hit_t g_hit[D_COUNT];
static bd_t d_boomy, d_punchy, d_tom[3], d_conga;
static sn_t d_snare, d_rim;
static hat_t d_hat, d_open, d_cym;
static svf_t d_clap_f, d_shake_f, d_bell_f;
static osc_t d_bell[2];
static float d_clap_fall, d_clap_env, d_clap_tail, d_shake_env, d_shake_dec, d_bell_env;
static int32_t d_clap_n;
static uint8_t d_kick_kind;

static void strike(int d, int seat, float vel, int kit_no, int note)
{
    const kit_t *k;
    hit_t *h;
    float tune, acc, life = 1.0f;
    if (d < 0 || d >= D_COUNT)
        return;
    k = &KIT[kit_no >= 0 && kit_no < K_COUNT ? kit_no : 0];
    h = &g_hit[d];
    tune = semis((float)note) * g_drop;
    acc = 0.35f + 0.65f * vel;
    h->seat = (uint8_t)seat;
    h->lvl = 0.9f;
    h->drive = k->drive;
    h->crush = k->crush;
    switch (d) {
    case D_KICK: case D_LONG_KICK: case D_HARD_KICK: {
        int punchy = d == D_HARD_KICK || (d == D_KICK && k->kick_synth);
        float dec = d == D_LONG_KICK ? fm_maxf(k->kick_dec, 0.8f) : k->kick_dec;
        g_hit[D_KICK].left = g_hit[D_LONG_KICK].left = g_hit[D_HARD_KICK].left = 0;
        d_kick_kind = (uint8_t)punchy;
        if (punchy) {
            bd_set(&d_punchy, k->kick_f * tune, dec, k->kick_tone, 0.6f, d == D_HARD_KICK ? 0.6f : 0.2f, acc);
            bd_trig(&d_punchy);
            if (d == D_HARD_KICK)
                h->drive = fm_maxf(h->drive, 0.5f);
        } else {
            bd_set(&d_boomy, k->kick_f * tune, dec, k->kick_tone, 0.5f, 0.3f, acc);
            bd_trig(&d_boomy);
        }
        life = 0.5f + dec * 2.5f;
        h->lvl = punchy ? 1.0f : 1.6f;
        break;
    }
    case D_SNARE:
        sn_set(&d_snare, k->sn_f * tune, k->sn_dec, k->sn_snap, k->sn_tone, acc);
        life = 0.3f + k->sn_dec * 1.5f;
        break;
    case D_RIM:
        sn_set(&d_rim, k->sn_f * 2.0f * tune, 0.12f, 0.15f, 0.7f, acc);
        life = 0.3f;
        h->lvl = 0.8f;
        break;
    case D_CLAP: case D_LONG_CLAP:
        g_hit[D_CLAP].left = g_hit[D_LONG_CLAP].left = 0;
        d_clap_n = 0;
        d_clap_env = acc;
        d_clap_tail = d == D_LONG_CLAP ? fm_maxf(k->clap_tail, 0.8f) : k->clap_tail;
        svf_set(&d_clap_f, 1100.0f * tune, 0.55f);
        life = d_clap_tail * 4.0f + 0.1f;
        break;
    case D_CLOSED_HAT: case D_PEDAL_HAT:
        g_hit[D_OPEN_HAT].left = 0;     /* closing the hat stops the open one */
        g_hit[D_CLOSED_HAT].left = g_hit[D_PEDAL_HAT].left = 0;
        hat_set(&d_hat, k->hat_f * tune, d == D_PEDAL_HAT ? k->hat_dec * 0.5f : k->hat_dec,
                d == D_PEDAL_HAT ? k->hat_tone * 0.7f : k->hat_tone, k->hat_noise, acc);
        life = 0.4f;
        h->lvl = d == D_PEDAL_HAT ? 1.2f : 0.7f;
        break;
    case D_OPEN_HAT:
        hat_set(&d_open, k->hat_f * tune, clampf(k->hat_dec + 0.45f, 0.0f, 1.0f), k->hat_tone, k->hat_noise, acc);
        life = 1.2f;
        h->lvl = 0.5f;
        break;
    case D_CRASH: case D_RIDE:
        g_hit[D_CRASH].left = g_hit[D_RIDE].left = 0;
        hat_set(&d_cym, (d == D_CRASH ? k->hat_f * 0.8f : k->hat_f * 1.25f) * tune, d == D_CRASH ? 0.95f : 0.8f,
                d == D_CRASH ? 0.7f : 0.85f, d == D_CRASH ? 0.9f : 0.35f, acc);
        life = d == D_CRASH ? 2.5f : 1.5f;
        h->lvl = 0.3f;
        break;
    case D_LOW_TOM: case D_MID_TOM: case D_HIGH_TOM: {
        static const float up[3] = {1.0f, 1.34f, 1.68f};
        bd_t *t = &d_tom[d - D_LOW_TOM];
        bd_set(t, k->tom_f * up[d - D_LOW_TOM] * tune, k->tom_dec, 0.35f, 0.25f, 0.1f, acc);
        bd_trig(t);
        life = 0.4f + k->tom_dec * 2.0f;
        h->lvl = 1.2f;
        break;
    }
    case D_SHAKER: case D_CLICK:
        g_hit[D_SHAKER].left = g_hit[D_CLICK].left = 0;
        d_shake_env = acc;
        d_shake_dec = d == D_SHAKER ? 0.035f : 0.004f;
        svf_set(&d_shake_f, (d == D_SHAKER ? 5200.0f : 2400.0f) * tune, 0.2f);
        life = 0.25f;
        h->lvl = d == D_SHAKER ? 0.5f : 0.8f;
        break;
    case D_COWBELL:
        osc_freq(&d_bell[0], 540.0f * tune);
        osc_freq(&d_bell[1], 800.0f * tune);
        d_bell_env = acc;
        life = 0.6f;
        h->lvl = 0.5f;
        break;
    case D_CONGA:
        bd_set(&d_conga, 210.0f * tune, 0.22f, 0.6f, 0.15f, 0.05f, acc);
        bd_trig(&d_conga);
        life = 0.6f;
        break;
    default:
        return;
    }
    h->left = (int32_t)(life * SR);
    h->n = 0;
}

static inline float drum_sample(int d)
{
    switch (d) {
    case D_KICK: case D_LONG_KICK: case D_HARD_KICK:
        return d_kick_kind ? bd_run(&d_punchy) : bd_run(&d_boomy);
    case D_SNARE:
        return sn_run(&d_snare);
    case D_RIM:
        return sn_run(&d_rim);
    case D_CLAP: case D_LONG_CLAP: {       /* three quick bursts 11 ms apart, then a tail */
        const int32_t burst = EN_SR * 11 / 1000;
        int32_t n = d_clap_n++;
        float e;
        if (n < 3 * burst) {
            e = 1.0f - (float)(n % burst) / (float)burst;
        } else {
            if (((n - 3 * burst) & 15) == 0)
                d_clap_fall = fm_exp2f(-1.4427f * (float)(n - 3 * burst) / (d_clap_tail * SR * 0.3f));
            e = d_clap_fall;
        }
        svf_run(&d_clap_f, frand());
        return d_clap_f.bd * e * d_clap_env * 1.6f;
    }
    case D_CLOSED_HAT: case D_PEDAL_HAT:
        return hat_run(&d_hat);
    case D_OPEN_HAT:
        return hat_run(&d_open);
    case D_CRASH: case D_RIDE:
        return hat_run(&d_cym);
    case D_LOW_TOM: case D_MID_TOM: case D_HIGH_TOM:
        return bd_run(&d_tom[d - D_LOW_TOM]);
    case D_SHAKER: case D_CLICK:
        d_shake_env *= 1.0f - 1.0f / (d_shake_dec * SR);
        svf_run(&d_shake_f, frand());
        return d_shake_f.hi * d_shake_env;
    case D_COWBELL:
        d_bell_env *= 1.0f - 1.0f / (0.09f * SR);
        svf_run(&d_bell_f, osc_run(&d_bell[0]) + osc_run(&d_bell[1]));
        return d_bell_f.bd * d_bell_env * 1.5f;
    case D_CONGA:
        return bd_run(&d_conga);
    default:
        return 0.0f;
    }
}

/* "Found" and "Click" can also be recordings: which one comes in the note */
static const uint8_t FOUND[] = {S_COINS, S_TING, S_WOOD, S_PAPER, S_PING, S_GLASS, S_TICK, S_POP, S_SNAP};
static patch_t g_found = {"Found", E_SAMPLE, 0, 1200, 0.9f, 0.1f, 0.004f, 0.25f, 1.0f, 0.05f, 0, 0, 1,
                          0, 0, 0, 0, 6000.0f, 0.1f, 0, 0.2f, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0, 0, 0, 0, 0};

/* -------------------------------------------------------------------------------- the engine --- */
#define QN 64
static en_cmd_t q[QN];
static volatile uint32_t q_w, q_r;
#define BARRIER() __asm__ volatile("" ::: "memory")

void en_post(const en_cmd_t *c)
{
    if (q_w - q_r >= QN)
        return;                     /* full: the note is lost rather than the beat */
    q[q_w % QN] = *c;
    BARRIER();
    q_w++;
}

void en_note(int seat, int sound, int note, int vel, int arg)
{
    en_cmd_t c;
    c.op = EN_NOTE;
    c.seat = (uint8_t)seat;
    c.sound = (uint8_t)sound;
    c.vel = (uint8_t)vel;
    c.note = (int16_t)note;
    c.arg = (uint16_t)arg;
    en_post(&c);
}

void en_set(int op, int seat, int vel, int note, int arg)
{
    en_cmd_t c;
    c.op = (uint8_t)op;
    c.seat = (uint8_t)seat;
    c.sound = 0;
    c.vel = (uint8_t)vel;
    c.note = (int16_t)note;
    c.arg = (uint16_t)arg;
    en_post(&c);
}

void en_init(void)
{
    int i;
    for (i = 0; i < EN_SEATS; i++) {
        g_level[i] = 1.0f;
        g_tone[i] = 1.0f;
    }
    for (i = 0; i < ECHO_MAX; i++)
        echo_buf[i] = 0;
    for (i = 0; i < N_ANALOG; i++) {
        g_analog[i].h.on = 0;
        g_analog[i].now = 0.0f;
    }
    for (i = 0; i < N_FM; i++)
        g_fm[i].h.on = 0;
    for (i = 0; i < N_STRING; i++)
        g_string[i].h.on = 0;
    for (i = 0; i < N_ORGAN; i++)
        g_organ[i].h.on = 0;
    for (i = 0; i < N_VOX; i++) {
        g_vox[i].h.on = 0;
        g_vox[i].now = 0.0f;
    }
    for (i = 0; i < N_BOOM; i++) {
        g_boom[i].h.on = 0;
        g_boom[i].now = 0.0f;
        g_boom[i].life = 0;
    }
    for (i = 0; i < N_TAPE; i++)
        g_tape[i].h.on = 0;
    for (i = 0; i < N_HISS; i++)
        g_hiss[i].h.on = 0;
    svf_set(&d_clap_f, 1100.0f, 0.55f);
    svf_set(&d_shake_f, 5200.0f, 0.2f);
    svf_set(&d_bell_f, 900.0f, 0.6f);
    d_bell[0].w = d_bell[1].w = W_SQR;
    d_bell[0].pw = d_bell[1].pw = 0.5f;
    for (i = 0; i < D_COUNT; i++)
        g_hit[i].left = 0;
    q_w = q_r = 0;
}

static void note_on(const en_cmd_t *c)
{
    int seat = c->seat < EN_SEATS ? c->seat : 0;
    float vel = c->vel / 127.0f, freq;
    const patch_t *p;
    if (c->sound >= DRUM_BASE) {
        int d = c->sound - DRUM_BASE;
        if (d == D_FOUND || (d == D_CLICK && c->note > 0)) {
            int which = d == D_FOUND ? c->note : 5 + c->note;
            tape_t *v = take(g_tape, N_TAPE, sizeof(tape_t), 0);
            g_found.a = FOUND[which >= 0 && which < (int)sizeof FOUND ? which : 0];
            begin(&v->h, &g_found, seat, 1.0f, vel, 0);
            tape_start(v);
        } else {
            strike(d, seat, vel, c->arg, c->note);
        }
        return;
    }
    if (c->sound >= I_COUNT)
        return;
    p = &PATCH[c->sound];
    freq = mtof((float)c->note + 12.0f * p->oct);
    switch (p->eng) {
    case E_ANALOG: { analog_t *v = take(g_analog, N_ANALOG, sizeof(analog_t), p->glide > 0.0f); begin(&v->h, p, seat, freq, vel, c->arg); analog_start(v); break; }
    case E_FM: { fm_t *v = take(g_fm, N_FM, sizeof(fm_t), 0); begin(&v->h, p, seat, freq, vel, c->arg); fm_start(v); break; }
    case E_STRING: { pluck_t *v = take(g_string, N_STRING, sizeof(pluck_t), 0); begin(&v->h, p, seat, freq, vel, c->arg); pluck_start(v); break; }
    case E_ORGAN: { organ_t *v = take(g_organ, N_ORGAN, sizeof(organ_t), 0); begin(&v->h, p, seat, freq, vel, c->arg); organ_start(v); break; }
    case E_FORMANT: { vox_t *v = take(g_vox, N_VOX, sizeof(vox_t), p->glide > 0.0f || p->d > 0.0f); begin(&v->h, p, seat, freq, vel, c->arg); vox_start(v); break; }
    case E_BOOM: { boom_t *v = take(g_boom, N_BOOM, sizeof(boom_t), p->glide > 0.0f); begin(&v->h, p, seat, freq, vel, c->arg); boom_start(v); break; }
    case E_SAMPLE: { tape_t *v = take(g_tape, N_TAPE, sizeof(tape_t), 0); begin(&v->h, p, seat, p->b > 0.0f ? freq : 1.0f, vel, c->arg); tape_start(v); break; }
    case E_NOISE: { hiss_t *v = take(g_hiss, N_HISS, sizeof(hiss_t), 0); begin(&v->h, p, seat, freq, vel, c->arg); hiss_start(v); break; }
    default: break;
    }
}

static void panic_now(void);
static void all_off(void)
{
    int i;
    for (i = 0; i < N_ANALOG; i++) g_analog[i].h.hold = 0;
    for (i = 0; i < N_FM; i++) g_fm[i].h.hold = 0;
    for (i = 0; i < N_ORGAN; i++) g_organ[i].h.hold = 0;
    for (i = 0; i < N_VOX; i++) g_vox[i].h.hold = 0;
    for (i = 0; i < N_TAPE; i++) g_tape[i].h.hold = 0;
    for (i = 0; i < N_HISS; i++) g_hiss[i].h.hold = 0;
    for (i = 0; i < N_STRING; i++) g_string[i].life = 0;
    for (i = 0; i < N_BOOM; i++) g_boom[i].life = 0;
}

static void command(const en_cmd_t *c)
{
    switch (c->op) {
    case EN_NOTE:
        note_on(c);
        break;
    case EN_SEAT_LEVEL:
        if (c->seat < EN_SEATS)
            g_level[c->seat] = c->vel / 127.0f * 1.27f;
        break;
    case EN_SEAT_TONE:
        if (c->seat < EN_SEATS)
            g_tone[c->seat] = fm_exp2f((c->vel - 64) / 32.0f);
        break;
    case EN_MASTER:
        g_master = c->vel / 127.0f;
        break;
    case EN_DROP:
        g_drop = semis(-(float)c->note / 16.0f);
        break;
    case EN_ECHO:
        g_echo = c->vel / 127.0f;
        if (c->arg) {
            int n = (int)((float)c->arg * (SR / 1000.0f));
            echo_len = n < 400 ? 400 : n > ECHO_MAX ? ECHO_MAX : n;
        }
        break;
    case EN_FX:
        if (c->note == 0)
            g_drive = c->vel / 127.0f;
        else if (c->note == 1)
            g_crush = c->vel / 127.0f;
        else if (c->note == 2 && c->arg) {
            int n = (int)((float)c->arg * (SR / 1000.0f));
            echo_len = n < 400 ? 400 : n > ECHO_MAX ? ECHO_MAX : n;
        } else if (c->note == 3)
            g_feedback = c->vel / 127.0f * 0.9f;
        break;
    case EN_ALL_OFF:
        all_off();
        break;
    case EN_PANIC:
        panic_now();
        break;
    case EN_SEAT_SEND:
        if (c->seat < EN_SEATS)
            g_send[c->seat] = c->vel / 127.0f;
        break;
    default:
        break;
    }
}

static void drain(void)
{
    while (q_r != q_w) {
        en_cmd_t c = q[q_r % QN];
        BARRIER();
        q_r++;
        command(&c);
    }
}

#define MIX(pool, n, tune, run)                                                             \
    for (i = 0; i < (n); i++) {                                                             \
        if (!pool[i].h.on)                                                                  \
            continue;                                                                       \
        if (ctl)                                                                            \
            tune;                                                                           \
        s = run * pool[i].h.p->lvl * (0.3f + 0.7f * pool[i].h.vel) * g_level[pool[i].h.seat]; \
        dry += s;                                                                           \
        wet += s * (pool[i].h.p->send + g_send[pool[i].h.seat]);                             \
    }

static float render_one(void)       /* one sample at 22,050 Hz, -1 .. 1 */
{
    int ctl = (g_tick++ % CTL) == 0, i, d;
    float dry = 0.0f, wet = 0.0f, s, back, in;
    MIX(g_analog, N_ANALOG, analog_tune(&g_analog[i]), analog_run(&g_analog[i]))
    MIX(g_fm, N_FM, fm_tune(&g_fm[i]), fm_run(&g_fm[i]))
    MIX(g_string, N_STRING, pluck_tune(&g_string[i]), pluck_run(&g_string[i]))
    MIX(g_organ, N_ORGAN, (void)0, organ_run(&g_organ[i]))
    MIX(g_vox, N_VOX, vox_tune(&g_vox[i]), vox_run(&g_vox[i]))
    MIX(g_boom, N_BOOM, boom_tune(&g_boom[i]), boom_run(&g_boom[i]))
    MIX(g_tape, N_TAPE, tape_tune(&g_tape[i]), tape_run(&g_tape[i]))
    MIX(g_hiss, N_HISS, hiss_tune(&g_hiss[i]), hiss_run(&g_hiss[i]))
    for (d = 0; d < D_COUNT; d++) {
        hit_t *h = &g_hit[d];
        if (h->left <= 0)
            continue;
        s = drum_sample(d);
        if (h->drive > 0.0f)
            s = softclip(s * (1.0f + h->drive * 4.0f));
        if (h->crush > 0.0f) {
            if (++h->n >= 1 + (int)(h->crush * 6.0f)) {
                h->n = 0;
                h->held = s;
            }
            s = h->held;
        }
        if (h->left < 256)          /* a short fade so nothing stops with a click */
            s *= h->left * (1.0f / 256.0f);
        h->left--;
        dry += s * h->lvl * g_level[h->seat];
    }
    /* the global effects: drive, a crush, and the echo (one tap with feedback) */
    if (g_drive > 0.0f)
        dry = softclip(dry * (1.0f + g_drive * 5.0f)) * (1.0f - g_drive * 0.3f);
    if (g_crush > 0.0f) {
        static float held;
        static int n;
        if (++n >= 1 + (int)(g_crush * 8.0f)) {
            float qz = 3.0f + (1.0f - g_crush) * 40.0f;
            n = 0;
            held = fm_floorf(dry * qz + 0.5f) / qz;
        }
        dry = held;
    }
    back = echo_buf[echo_at] * (1.0f / 32768.0f);
    in = softclip(wet * g_echo + back * g_feedback);
    echo_buf[echo_at] = (int16_t)(in * 32000.0f);
    if (++echo_at >= echo_len)
        echo_at = 0;
    return softclip((dry + back) * g_master * 0.6f);
}

void en_render(int32_t *out, uint32_t n, uint32_t gain_q12)
{
    static float last;
    const float gain = (float)gain_q12 * (1.0f / 4096.0f) * 2.0f * 8300000.0f;
    uint32_t j;
    drain();
    for (j = 0; j < n; j += 2) {    /* 22,050 Hz, each sample twice with the step smoothed */
        float s = render_one(), mid = (last + s) * 0.5f;
        int32_t a = (int32_t)(mid * gain), b = (int32_t)(s * gain);
        last = s;
        out[2 * j] = out[2 * j + 1] = a;
        if (j + 1 < n)
            out[2 * j + 2] = out[2 * j + 3] = b;
    }
}

static void panic_now(void)
{
    int i;
    all_off();
    for (i = 0; i < N_ANALOG; i++) g_analog[i].h.on = 0;
    for (i = 0; i < N_FM; i++) g_fm[i].h.on = 0;
    for (i = 0; i < N_ORGAN; i++) g_organ[i].h.on = 0;
    for (i = 0; i < N_VOX; i++) g_vox[i].h.on = 0;
    for (i = 0; i < N_TAPE; i++) g_tape[i].h.on = 0;
    for (i = 0; i < N_HISS; i++) g_hiss[i].h.on = 0;
    for (i = 0; i < N_STRING; i++) g_string[i].h.on = 0;
    for (i = 0; i < N_BOOM; i++) g_boom[i].h.on = 0;
    for (i = 0; i < D_COUNT; i++) g_hit[i].left = 0;
    for (i = 0; i < ECHO_MAX; i++) echo_buf[i] = 0;
}

void en_panic(void) { en_set(EN_PANIC, 0, 0, 0, 0); }

void en_play(int on) { (void)on; }

int en_active(void)
{
    int n = 0, i;
    for (i = 0; i < N_ANALOG; i++) n += g_analog[i].h.on;
    for (i = 0; i < N_FM; i++) n += g_fm[i].h.on;
    for (i = 0; i < N_ORGAN; i++) n += g_organ[i].h.on;
    for (i = 0; i < N_VOX; i++) n += g_vox[i].h.on;
    for (i = 0; i < N_TAPE; i++) n += g_tape[i].h.on;
    for (i = 0; i < N_HISS; i++) n += g_hiss[i].h.on;
    for (i = 0; i < N_STRING; i++) n += g_string[i].h.on;
    for (i = 0; i < N_BOOM; i++) n += g_boom[i].h.on;
    for (i = 0; i < D_COUNT; i++) n += g_hit[i].left > 0;
    return n;
}

int en_shed(void)
{
    head_t *old = 0;
    int32_t *life = 0;
    int i;
#define OLDEST(pool, n, lifeexpr) for (i = 0; i < (n); i++) if (pool[i].h.on && (!old || pool[i].h.born < old->born)) { old = &pool[i].h; life = lifeexpr; }
    OLDEST(g_analog, N_ANALOG, 0) OLDEST(g_fm, N_FM, 0) OLDEST(g_organ, N_ORGAN, 0) OLDEST(g_vox, N_VOX, 0)
    OLDEST(g_tape, N_TAPE, 0) OLDEST(g_hiss, N_HISS, 0) OLDEST(g_string, N_STRING, &g_string[i].life)
    OLDEST(g_boom, N_BOOM, &g_boom[i].life)
    if (!old)
        return 0;
    old->hold = 0;
    old->on = (uint8_t)(life != 0);
    if (life)
        *life = 0;
    return 1;
}

int en_sound_by_name(const char *name)
{
    int i;
    for (i = 0; i < I_COUNT; i++) {
        const char *a = PATCH[i].name, *b = name;
        while (*a && *a == *b)
            a++, b++;
        if (*a == 0 && *b == 0)
            return i;
    }
    return -1;
}

const char *en_sound_name(int i) { return i >= 0 && i < I_COUNT ? PATCH[i].name : "?"; }
