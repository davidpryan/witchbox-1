// Playbox audio engine. See engine.h and SOUNDS.md.
//
// The synthesis building blocks are DaisySP (MIT licence, Electrosmith); its
// drum, string and modal models are ports of Mutable Instruments' Plaits
// (MIT licence, Emilie Gillet). DaisySP's source files are compiled into
// this one file, below, so that their rand() can be pointed at a generator
// that is safe on the second core. Licence texts: firmware/LICENSES.
//
// How it is organised: a few small pools of voices, one pool per kind of
// engine. A note picks a free voice in the right pool (or takes the oldest).
// Each instrument is a Patch: an engine and its settings. Drums are single
// instances that are simply struck again.
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "engine.h"
#include "ids.h"
#include "samples_data.h"

static uint32_t rng_state = 0x1234ABCD;
extern "C" int pb_rand(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (int)(rng_state & RAND_MAX);
}

// The library calls sinf() and powf(2, x) for every sample in several places.
// On the Pico those cost hundreds of processor cycles each, so they are
// pointed at table look-ups here: about 20 cycles, and exact enough to hear
// no difference (errors under 0.002 percent).
static float g_sin[1025], g_exp2[257];
static void fast_tables(void) {
    for (int i = 0; i <= 1024; i++) {
        g_sin[i] = sinf((float)i * (6.283185307f / 1024.0f));
    }
    for (int i = 0; i <= 256; i++) {
        g_exp2[i] = powf(2.0f, (float)i / 256.0f);
    }
}
extern "C" float pb_sinf(float x) {
    float t = x * 0.15915494f;                  // turns
    t -= (float)(int)t;
    if (t < 0.0f) {
        t += 1.0f;
    }
    t *= 1024.0f;
    int i = (int)t;
    return g_sin[i] + (g_sin[i + 1] - g_sin[i]) * (t - (float)i);
}
static inline float fast_exp2(float x) {
    if (x < -100.0f) {
        return 0.0f;
    }
    if (x > 100.0f) {
        x = 100.0f;
    }
    int whole = (int)x;
    if (x < (float)whole) {
        whole--;
    }
    float t = (x - (float)whole) * 256.0f;
    int i = (int)t;
    union { uint32_t u; float f; } scale;
    scale.u = (uint32_t)(whole + 127) << 23;    // 2 to the power of whole
    return (g_exp2[i] + (g_exp2[i + 1] - g_exp2[i]) * (t - (float)i)) * scale.f;
}
static float slow_powf(float b, float x) { return powf(b, x); }
extern "C" float pb_powf(float b, float x) {
    if (b == 2.0f) {
        return fast_exp2(x);
    }
    if (x == 0.25f) {
        return sqrtf(sqrtf(b));
    }
    return slow_powf(b, x);
}
namespace std { using ::pb_rand; using ::pb_sinf; using ::pb_powf; }   // so std:: uses follow too
#define rand pb_rand
#define sinf pb_sinf
#define powf pb_powf

#include "Utility/dsp.h"
#include "Utility/dcblock.cpp"
#include "Dynamics/crossfade.cpp"
#include "Control/adsr.cpp"
#include "Filters/svf.cpp"
#include "Synthesis/oscillator.cpp"
#include "Synthesis/fm2.cpp"
#include "Synthesis/formantosc.cpp"
#include "Drums/analogbassdrum.cpp"
#include "Drums/analogsnaredrum.cpp"
#include "Drums/synthbassdrum.cpp"
#include "Drums/synthsnaredrum.cpp"
#include "Drums/hihat.cpp"

using namespace daisysp;

#define SR ((float)PB_SAMPLE_RATE)
#define CTL 16                      // slow things (glide, filter moves) are updated every CTL samples

static inline float frand() { return pb_rand() * kRandFrac * 2.0f - 1.0f; }
static inline float semis(float s) { return powf(2.0f, s * (1.0f / 12.0f)); }

// ---------------------------------------------------------------- patches ---

enum { E_ANALOG, E_FM, E_STRING, E_ORGAN, E_FORMANT, E_BOOM, E_SAMPLE, E_NOISE };
static const char *const kEngineNames[] = {"analog", "fm", "string", "organ", "voice",
                                           "boom", "sample", "noise"};
enum { SAW = Oscillator::WAVE_POLYBLEP_SAW, SQR = Oscillator::WAVE_POLYBLEP_SQUARE,
       TRI = Oscillator::WAVE_TRI, SIN = Oscillator::WAVE_SIN };

struct Patch {
    uint8_t eng = E_ANALOG;
    int8_t oct = 0;             // octaves up or down from the note asked for
    uint16_t gate = 250;        // ms a note is held when the toy does not say
    float lvl = 0.8f;
    float send = 0.0f;          // into the echo
    // amplitude envelope (analog, fm, organ, voice, sample, noise)
    float A = 0.004f, D = 0.25f, S = 0.7f, R = 0.25f;
    // analog
    uint8_t w1 = SAW, w2 = SAW, uni = 1;    // uni: saws stacked (1..4)
    float det = 0.0f;           // osc 2, in semitones away from osc 1
    float mix2 = 0.0f, sub = 0.0f, noise = 0.0f;
    float cut = 6000.0f, res = 0.1f;
    float env = 0.0f, fdec = 0.2f;          // filter envelope: octaves, and its fall time
    float glide = 0.0f, drive = 0.0f, spread = 0.0f, vib = 0.0f, pw = 0.5f, crush = 0.0f;
    float bend = 0.0f;          // pitch slides this many semitones over fdec
    float fatk = 0.0f;          // filter opens over this long instead of falling (pads that bloom)
    // the other engines read these five, each in its own way (see below)
    float a = 0, b = 0, c = 0, d = 0, e = 0;
};

// fm:      a ratio, b index at the start, c index it settles to, d fall time of the index
// string:  a how hard the pluck is (0 soft .. 1 sharp), b brightness of the string, c how long it rings (0..1)
// organ:   a..e level of the drawbars at 1/2, 1, 2, 3 and 4 times the note
// voice:   a, b the two formant frequencies; c breath; d how long it slides up into the note
// boom:    a decay, b tone, c pitch punch, d life in seconds
// sample:  a which recording, b its own pitch as a MIDI note (0 = always play as recorded), c 1 to loop
// noise:   a 0 low-pass, 1 high-pass, 2 band-pass; b filter start in Hz; c filter end in Hz (swept over the hold)

static const Patch kPatches[I_COUNT] = {
    /* Sub */           {.oct = -1, .gate = 300, .lvl = 0.9f, .D = 0.4f, .S = 0.9f, .R = 0.15f, .w1 = SIN, .w2 = SIN,
                         .det = 0.08f, .mix2 = 0.5f, .cut = 500.0f},
    /* 808 Boom */      {.eng = E_BOOM, .oct = -1, .lvl = 5.0f, .a = 0.75f, .b = 0.25f, .c = 0.35f, .d = 2.2f},
    /* 808 Slide */     {.eng = E_BOOM, .oct = -1, .lvl = 5.0f, .glide = 0.12f, .a = 0.7f, .b = 0.3f, .c = 0.3f, .d = 2.0f},
    /* Acid */          {.oct = -1, .gate = 110, .D = 0.15f, .S = 0.6f, .R = 0.06f, .cut = 300.0f, .res = 0.82f,
                         .env = 3.5f, .fdec = 0.14f, .glide = 0.05f, .drive = 0.3f},
    /* Reese */         {.oct = -1, .gate = 500, .A = 0.02f, .S = 0.9f, .R = 0.2f, .det = 0.22f, .mix2 = 1.0f,
                         .cut = 420.0f, .res = 0.25f, .drive = 0.2f},
    /* Saw Bass */      {.oct = -1, .gate = 120, .D = 0.2f, .S = 0.5f, .R = 0.08f, .sub = 0.5f, .cut = 900.0f,
                         .res = 0.2f, .env = 2.0f, .fdec = 0.12f},
    /* FM Bass */       {.eng = E_FM, .oct = -1, .gate = 160, .D = 0.3f, .S = 0.4f, .R = 0.1f, .a = 1.0f, .b = 3.5f,
                         .c = 0.6f, .d = 0.09f},
    /* Pluck Bass */    {.eng = E_STRING, .oct = -1, .lvl = 1.1f, .a = 0.4f, .b = 0.35f, .c = 0.55f},
    /* Soft Keys */     {.eng = E_FM, .gate = 260, .lvl = 0.7f, .send = 0.15f, .A = 0.006f, .D = 0.6f, .S = 0.35f,
                         .R = 0.35f, .a = 1.0f, .b = 1.4f, .c = 0.3f, .d = 0.25f},
    /* Electric Piano */{.eng = E_FM, .gate = 300, .lvl = 0.7f, .send = 0.12f, .D = 0.9f, .S = 0.25f, .R = 0.4f,
                         .a = 14.0f, .b = 1.1f, .c = 0.08f, .d = 0.12f},
    /* Dull Piano */    {.eng = E_STRING, .lvl = 1.2f, .send = 0.25f, .vib = 0.12f, .a = 0.15f, .b = 0.2f, .c = 0.8f},
    /* Toy Piano */     {.eng = E_FM, .oct = 1, .gate = 40, .lvl = 0.6f, .A = 0.001f, .D = 0.5f, .S = 0.0f, .R = 0.35f,
                         .a = 4.0f, .b = 2.4f, .c = 0.3f, .d = 0.07f},
    /* Clav */          {.gate = 140, .D = 0.25f, .S = 0.2f, .R = 0.08f, .w1 = SQR, .cut = 2400.0f, .res = 0.3f,
                         .env = 1.5f, .fdec = 0.1f, .pw = 0.22f},
    /* Music Box */     {.eng = E_FM, .oct = 2, .gate = 40, .lvl = 0.55f, .send = 0.2f, .A = 0.001f, .D = 1.1f, .S = 0.0f,
                         .R = 0.8f, .a = 5.0f, .b = 1.3f, .c = 0.15f, .d = 0.2f},
    /* Soul Organ */    {.eng = E_ORGAN, .gate = 260, .lvl = 0.6f, .A = 0.004f, .S = 1.0f, .R = 0.06f, .a = 0.7f,
                         .b = 1.0f, .c = 0.6f, .d = 0.0f, .e = 0.15f},
    /* Church Organ */  {.eng = E_ORGAN, .gate = 900, .lvl = 0.5f, .send = 0.3f, .A = 0.08f, .S = 1.0f, .R = 0.5f,
                         .a = 0.8f, .b = 1.0f, .c = 0.8f, .d = 0.5f, .e = 0.6f},
    /* Choir Aah */     {.eng = E_FORMANT, .gate = 900, .lvl = 0.7f, .send = 0.3f, .A = 0.15f, .S = 0.9f, .R = 0.6f,
                         .vib = 0.15f, .a = 800.0f, .b = 1200.0f, .c = 0.1f, .d = 0.0f},
    /* Choir Ooh */     {.eng = E_FORMANT, .gate = 900, .lvl = 0.7f, .send = 0.3f, .A = 0.2f, .S = 0.9f, .R = 0.7f,
                         .vib = 0.12f, .a = 380.0f, .b = 820.0f, .c = 0.25f, .d = 0.0f},
    /* Warm Pad */      {.gate = 900, .lvl = 0.55f, .send = 0.2f, .A = 0.25f, .D = 0.6f, .S = 0.8f, .R = 0.8f,
                         .det = 0.09f, .mix2 = 1.0f, .cut = 1100.0f, .res = 0.15f},
    /* Dark Strings */  {.gate = 900, .lvl = 0.5f, .send = 0.25f, .A = 0.3f, .S = 0.9f, .R = 0.9f, .uni = 4,
                         .cut = 700.0f, .spread = 0.14f},
    /* Glass Pad */     {.eng = E_FM, .oct = 1, .gate = 900, .lvl = 0.45f, .send = 0.35f, .A = 0.3f, .D = 1.0f,
                         .S = 0.7f, .R = 1.0f, .a = 3.01f, .b = 1.2f, .c = 0.5f, .d = 0.8f},
    /* Saw Pad */       {.gate = 700, .lvl = 0.5f, .send = 0.2f, .A = 0.08f, .S = 0.9f, .R = 0.5f, .uni = 4,
                         .cut = 3200.0f, .spread = 0.16f},
    /* Coven Saws */    {.gate = 1800, .lvl = 0.5f, .send = 0.25f, .A = 0.05f, .S = 1.0f, .R = 0.6f, .uni = 4,
                         .cut = 450.0f, .res = 0.2f, .env = 2.6f, .spread = 0.3f, .fatk = 1.6f},
    /* Drone */         {.oct = -1, .gate = 4000, .lvl = 0.6f, .A = 0.6f, .S = 1.0f, .R = 1.5f, .det = 0.05f,
                         .mix2 = 1.0f, .sub = 0.5f, .cut = 350.0f, .res = 0.3f, .env = 1.5f, .fatk = 3.0f},
    /* Saw Lead */      {.gate = 180, .lvl = 0.6f, .send = 0.2f, .D = 0.3f, .S = 0.7f, .R = 0.15f, .det = 0.1f,
                         .mix2 = 0.8f, .cut = 3500.0f, .res = 0.15f, .env = 0.8f},
    /* Square Lead */   {.gate = 180, .lvl = 0.55f, .send = 0.2f, .S = 0.8f, .R = 0.12f, .w1 = SQR, .cut = 3000.0f,
                         .vib = 0.1f, .pw = 0.4f},
    /* Chip Lead */     {.gate = 110, .lvl = 0.5f, .S = 1.0f, .R = 0.02f, .w1 = SQR, .cut = 9000.0f, .pw = 0.25f,
                         .crush = 0.55f},
    /* Glide Lead */    {.gate = 700, .lvl = 0.6f, .send = 0.3f, .A = 0.05f, .S = 0.9f, .R = 0.4f, .w1 = TRI, .w2 = SIN,
                         .mix2 = 0.6f, .cut = 2500.0f, .glide = 0.35f, .vib = 0.25f},
    /* Screamer */      {.oct = 1, .gate = 90, .lvl = 0.55f, .send = 0.15f, .S = 0.9f, .R = 0.05f, .w2 = SQR, .det = 7.02f,
                         .mix2 = 0.7f, .noise = 0.15f, .cut = 2600.0f, .res = 0.55f, .env = 1.5f, .fdec = 0.2f, .drive = 0.9f},
    /* Whistle */       {.oct = 1, .gate = 320, .lvl = 0.5f, .send = 0.35f, .A = 0.04f, .S = 0.9f, .R = 0.3f, .w1 = SIN,
                         .w2 = SIN, .noise = 0.05f, .cut = 5000.0f, .vib = 0.18f},
    /* Glass Bell */    {.eng = E_FM, .oct = 1, .gate = 60, .lvl = 0.6f, .send = 0.4f, .A = 0.001f, .D = 1.6f, .S = 0.0f,
                         .R = 1.2f, .a = 3.52f, .b = 2.2f, .c = 0.3f, .d = 0.6f},
    /* Kalimba */       {.eng = E_FM, .oct = 1, .gate = 40, .lvl = 0.7f, .send = 0.15f, .A = 0.001f, .D = 0.7f, .S = 0.0f,
                         .R = 0.5f, .a = 3.0f, .b = 1.6f, .c = 0.05f, .d = 0.05f},
    /* Marimba */       {.eng = E_FM, .gate = 40, .lvl = 0.8f, .send = 0.1f, .A = 0.001f, .D = 0.45f, .S = 0.0f, .R = 0.3f,
                         .a = 3.0f, .b = 2.0f, .c = 0.0f, .d = 0.035f},
    /* Harp */          {.eng = E_STRING, .lvl = 1.1f, .send = 0.2f, .a = 0.3f, .b = 0.6f, .c = 0.85f},
    /* Guitar */        {.eng = E_STRING, .lvl = 1.1f, .send = 0.1f, .a = 0.7f, .b = 0.5f, .c = 0.7f},
    /* Trap Bell */     {.eng = E_FM, .oct = 1, .gate = 50, .lvl = 0.55f, .send = 0.25f, .A = 0.001f, .D = 0.5f, .S = 0.0f,
                         .R = 0.4f, .a = 5.0f, .b = 1.8f, .c = 0.1f, .d = 0.15f},
    /* Rave Stab */     {.gate = 120, .lvl = 0.5f, .send = 0.2f, .D = 0.2f, .S = 0.5f, .R = 0.1f, .uni = 3,
                         .cut = 1800.0f, .res = 0.35f, .env = 2.0f, .fdec = 0.12f, .drive = 0.3f, .spread = 0.2f},
    /* Minor Stab */    {.gate = 140, .lvl = 0.55f, .send = 0.15f, .D = 0.25f, .S = 0.3f, .R = 0.12f, .det = 0.08f,
                         .mix2 = 0.8f, .cut = 1400.0f, .res = 0.2f, .env = 1.6f, .fdec = 0.15f},
    /* Ghost Voice */   {.eng = E_FORMANT, .oct = 1, .gate = 450, .lvl = 0.6f, .send = 0.45f, .A = 0.06f, .S = 0.9f,
                         .R = 0.5f, .glide = 0.12f, .vib = 0.2f, .a = 650.0f, .b = 1080.0f, .c = 0.3f, .d = 0.08f},
    /* Screwed Voice */ {.eng = E_FORMANT, .oct = -1, .gate = 1600, .lvl = 0.75f, .send = 0.3f, .A = 0.2f, .S = 0.9f,
                         .R = 0.9f, .glide = 0.3f, .vib = 0.1f, .a = 500.0f, .b = 800.0f, .c = 0.15f, .d = 0.35f},
    /* Crackle */       {.eng = E_SAMPLE, .gate = 2400, .lvl = 5.0f, .A = 0.05f, .S = 1.0f, .R = 0.3f, .a = S_CRACKLE, .c = 1},
    /* Rain */          {.eng = E_NOISE, .gate = 2400, .lvl = 0.25f, .A = 0.3f, .S = 1.0f, .R = 0.6f, .a = 1, .b = 5000.0f, .c = 5000.0f},
    /* Rumble */        {.eng = E_NOISE, .gate = 2400, .lvl = 0.9f, .A = 0.4f, .S = 1.0f, .R = 0.8f, .a = 0, .b = 90.0f, .c = 90.0f},
    /* City Hum */      {.eng = E_SAMPLE, .gate = 2400, .lvl = 2.5f, .A = 0.3f, .S = 1.0f, .R = 0.6f, .a = S_HUM, .c = 1},
    /* Riser */         {.eng = E_NOISE, .gate = 2000, .lvl = 0.45f, .send = 0.2f, .A = 1.0f, .S = 1.0f, .R = 0.05f,
                         .res = 0.5f, .a = 2, .b = 250.0f, .c = 7000.0f},
    /* Impact */        {.eng = E_SAMPLE, .gate = 1000, .lvl = 1.0f, .send = 0.3f, .S = 1.0f, .a = S_IMPACT},
    /* Woosh */         {.eng = E_SAMPLE, .gate = 2500, .lvl = 0.8f, .send = 0.2f, .S = 1.0f, .a = S_WOOSH},
    /* Door */          {.eng = E_SAMPLE, .gate = 1200, .lvl = 0.8f, .send = 0.3f, .S = 1.0f, .a = S_DOOR},
    /* Crow */          {.eng = E_SAMPLE, .gate = 500, .lvl = 0.6f, .send = 0.5f, .S = 1.0f, .a = S_CROW},
    /* Skip */          {.eng = E_SAMPLE, .gate = 270, .lvl = 0.8f, .S = 1.0f, .a = S_SKIP},
    /* Vox Hit */       {.eng = E_SAMPLE, .gate = 200, .lvl = 0.9f, .send = 0.25f, .S = 1.0f, .R = 0.08f, .a = S_VOX_HIT, .b = 36},
};

// One row of settings for each kit; the drum code below reads them.
struct Kit {
    float kick_f, kick_dec, kick_tone;  uint8_t kick_synth;     // synth = the punchy model, else the boomy one
    float sn_f, sn_dec, sn_snap, sn_tone;
    float hat_f, hat_dec, hat_tone, hat_noise;
    float tom_f, tom_dec;
    float clap_tail;            // seconds
    float drive, crush;
};
static const Kit kKits[K_COUNT] = {
    /* 808 */      {48, 0.55f, 0.20f, 0, 190, 0.30f, 0.60f, 0.5f, 3300, 0.22f, 0.55f, 0.80f, 95, 0.45f, 0.25f, 0.0f, 0.0f},
    /* 909 */      {55, 0.45f, 0.55f, 1, 220, 0.25f, 0.75f, 0.7f, 4200, 0.18f, 0.75f, 0.90f, 110, 0.35f, 0.20f, 0.2f, 0.0f},
    /* Acoustic */ {62, 0.30f, 0.45f, 1, 200, 0.35f, 0.85f, 0.6f, 3600, 0.25f, 0.60f, 1.00f, 120, 0.40f, 0.15f, 0.0f, 0.0f},
    /* Lo-fi */    {52, 0.45f, 0.15f, 0, 175, 0.28f, 0.50f, 0.3f, 2600, 0.18f, 0.30f, 0.85f, 90, 0.40f, 0.20f, 0.3f, 0.5f},
    /* Witch */    {40, 0.85f, 0.15f, 0, 160, 0.50f, 0.70f, 0.4f, 3000, 0.25f, 0.45f, 0.90f, 70, 0.75f, 0.90f, 0.15f, 0.0f},
    /* Garage */   {58, 0.35f, 0.50f, 1, 240, 0.18f, 0.55f, 0.8f, 4600, 0.14f, 0.80f, 0.90f, 115, 0.30f, 0.15f, 0.1f, 0.0f},
    /* Chip */     {70, 0.25f, 0.80f, 1, 280, 0.15f, 0.90f, 0.9f, 5200, 0.10f, 0.90f, 0.60f, 140, 0.25f, 0.10f, 0.0f, 0.75f},
    /* Tin */      {80, 0.20f, 0.70f, 1, 330, 0.12f, 0.40f, 0.9f, 5600, 0.12f, 0.95f, 0.50f, 180, 0.20f, 0.10f, 0.4f, 0.3f},
};

// ----------------------------------------------------------------- shared ---

static float g_level[PB_SEATS], g_tone[PB_SEATS];
static float g_master = 0.8f, g_drop = 1.0f;
static float g_echo = 0.5f;
static uint32_t g_clock;            // counts notes, to find the oldest voice
static uint32_t g_tick;             // counts samples

#define ECHO_MAX (PB_SAMPLE_RATE * 2 / 5)       // 0.4 s
static int16_t g_echo_buf[ECHO_MAX];
static int g_echo_len = PB_SAMPLE_RATE / 4, g_echo_at;

struct Head {                       // what every voice has
    bool on;
    uint8_t seat;
    const Patch *p;
    float vel, freq;
    int32_t hold;                   // samples until the note is let go
    uint32_t born;
};

template <typename V, int N>
static V *take(V (&pool)[N]) {
    V *best = &pool[0];
    for (int i = 0; i < N; i++) {
        if (!pool[i].h.on) {
            return &pool[i];
        }
        if (pool[i].h.born < best->h.born) {
            best = &pool[i];
        }
    }
    return best;
}

static void begin(Head &h, const Patch *p, int seat, float freq, float vel, int hold_ms) {
    h.on = true;
    h.seat = seat;
    h.p = p;
    h.vel = vel;
    h.freq = freq;
    h.hold = (int32_t)((hold_ms ? hold_ms : p->gate) * (SR / 1000.0f));
    h.born = ++g_clock;
}

static void set_env(Adsr &e, const Patch *p) {
    e.SetAttackTime(p->A);
    e.SetDecayTime(p->D);
    e.SetSustainLevel(p->S);
    e.SetReleaseTime(p->R);
    e.Retrigger(false);
}

// ----------------------------------------------------------------- analog ---

struct Analog {
    Head h;
    Oscillator o[4];
    Svf f;
    Adsr amp;
    float now, sub_ph, fenv, fstep, lfo, held;
    int crush_n;

    void start() {
        const Patch *p = h.p;
        if (p->glide <= 0.0f || now <= 0.0f) {
            now = h.freq;
        }
        o[0].SetWaveform(p->w1);
        o[1].SetWaveform(p->w2);
        o[2].SetWaveform(p->w1);
        o[3].SetWaveform(p->w1);
        for (int i = 0; i < 4; i++) {
            o[i].SetAmp(0.5f);
            o[i].SetPw(p->pw);
        }
        f.SetRes(p->res);
        f.SetDrive(0.0f);
        set_env(amp, p);
        if (p->fatk > 0.0f) {                   // the filter opens
            fenv = 0.0f;
            fstep = CTL / (p->fatk * SR);
        } else {                                // the filter closes
            fenv = 1.0f;
            fstep = -1.0f;
        }
        tune();
    }
    void tune() {                               // every CTL samples
        const Patch *p = h.p;
        if (p->glide > 0.0f) {
            now += (h.freq - now) * (CTL / (p->glide * SR * 0.35f + CTL));
        }
        lfo += CTL * 5.5f / SR;
        if (lfo > 1.0f) {
            lfo -= 1.0f;
        }
        float fr = now * g_drop;
        if (p->vib > 0.0f) {
            fr *= 1.0f + p->vib * 0.06f * sinf(lfo * TWOPI_F);
        }
        if (p->bend != 0.0f) {
            fr *= semis(p->bend * (1.0f - fenv));
        }
        if (fstep < 0.0f) {
            fenv *= 1.0f - CTL / (p->fdec * SR);
        } else if (fenv < 1.0f) {
            fenv += fstep;
        }
        o[0].SetFreq(fr);
        if (p->uni > 1) {                       // a stack of saws, spread either side
            float s = p->spread * 0.06f;
            o[1].SetFreq(fr * (1.0f + s));
            o[2].SetFreq(fr * (1.0f - s * 0.62f));
            o[3].SetFreq(fr * (1.0f + s * 0.37f) * 0.5f);
        } else {
            o[1].SetFreq(fr * semis(p->det));
        }
        float cut = p->cut * g_tone[h.seat] * powf(2.0f, p->env * fenv);
        f.SetFreq(fclamp(cut, 30.0f, 7000.0f));
        sub_ph += 0.0f;
    }
    float run() {
        const Patch *p = h.p;
        float s = o[0].Process();
        if (p->uni > 1) {
            for (int i = 1; i < p->uni; i++) {
                s += o[i].Process();
            }
            s *= 0.6f;
        } else if (p->mix2 > 0.0f) {
            s += p->mix2 * o[1].Process();
        }
        if (p->sub > 0.0f) {
            sub_ph += now * g_drop * 0.5f / SR;
            if (sub_ph > 1.0f) {
                sub_ph -= 1.0f;
            }
            s += p->sub * sinf(sub_ph * TWOPI_F);
        }
        if (p->noise > 0.0f) {
            s += p->noise * frand();
        }
        if (p->drive > 0.0f) {
            s = SoftClip(s * (1.0f + p->drive * 6.0f)) * (1.0f - p->drive * 0.35f);
        }
        f.Process(s);
        s = f.Low();
        if (p->crush > 0.0f) {                  // hold each sample for a few, and round it off
            int n = 1 + (int)(p->crush * 10.0f);
            if (++crush_n >= n) {
                crush_n = 0;
                float q = 4.0f + (1.0f - p->crush) * 28.0f;
                held = floorf(s * q + 0.5f) / q;
            }
            s = held;
        }
        float a = amp.Process(h.hold > 0);
        if (h.hold > 0) {
            h.hold--;
        } else if (!amp.IsRunning()) {
            h.on = false;
            now = h.freq;
        }
        return s * a;
    }
};

// --------------------------------------------------------------------- fm ---

struct Fm {
    Head h;
    Fm2 fm;
    Adsr amp;
    float ienv;
    void start() {
        fm.SetRatio(h.p->a);
        ienv = 1.0f;
        set_env(amp, h.p);
        tune();
    }
    void tune() {
        const Patch *p = h.p;
        ienv *= 1.0f - CTL / (p->d * SR);
        fm.SetFrequency(h.freq * g_drop);
        fm.SetIndex((p->c + (p->b - p->c) * ienv) * (0.4f + 0.6f * h.vel) * g_tone[h.seat]);
    }
    float run() {
        float a = amp.Process(h.hold > 0);
        if (h.hold > 0) {
            h.hold--;
        } else if (!amp.IsRunning()) {
            h.on = false;
        }
        return fm.Process() * a;
    }
};

// ----------------------------------------------------------------- string ---
// A plucked string, the classic way: a burst of noise goes round a delay as
// long as one wave of the note, and is smoothed a little each time round.

#define STRING_MAX 1024             // longest delay: the lowest note is about 24 Hz

struct Pluck {
    Head h;
    float line[STRING_MAX];
    float len, lp, keep, bright, fade;
    int at, n;
    int32_t life;
    void start() {
        const Patch *p = h.p;
        len = fclamp(SR / (h.freq * g_drop) - 0.5f, 8.0f, STRING_MAX - 2.0f);
        n = (int)len + 2;
        float dull = 0.0f, sharp = 0.25f + 0.75f * p->a * (0.5f + 0.5f * h.vel);
        for (int i = 0; i < n; i++) {           // the pluck: noise, softened for gentle sounds
            dull += (frand() - dull) * sharp;
            line[i] = dull;
        }
        at = 0;
        lp = 0.0f;
        bright = fclamp(0.25f + 0.7f * p->b * g_tone[h.seat], 0.05f, 0.98f);
        // how much survives each trip round; low notes go round less often, so they lose less each time
        float secs = 0.3f + p->c * p->c * 6.0f;
        keep = pb_powf(2.0f, -10.0f * (len / SR) / secs);
        life = (int32_t)(secs * 1.2f * SR);
        fade = 1.0f;
    }
    void tune() {
        if (h.p->vib > 0.0f) {                  // a slow wobble, like worn tape
            len = fclamp(SR / (h.freq * g_drop * (1.0f + h.p->vib * 0.012f * pb_sinf(g_tick * (TWOPI_F * 0.7f / SR)))) - 0.5f,
                         8.0f, (float)(n - 2));
        }
    }
    float run() {
        float rd = (float)at - len;             // read one wave back, between two samples
        if (rd < 0.0f) {
            rd += (float)n;
        }
        int i = (int)rd;
        float fr = rd - (float)i;
        int j = i + 1 < n ? i + 1 : 0;
        float s = line[i] * (1.0f - fr) + line[j] * fr;
        lp += (s - lp) * bright;
        line[at] = lp * keep;
        if (++at >= n) {
            at = 0;
        }
        if (--life < 0) {
            fade -= 1.0f / (0.03f * SR);
            if (fade <= 0.0f) {
                h.on = false;
                return 0.0f;
            }
        }
        return s * fade;
    }
};

// ------------------------------------------------------------------ organ ---

struct Organ {
    Head h;
    Adsr amp;
    float ph[5];
    void start() { set_env(amp, h.p); }
    void tune() {}
    float run() {
        static const float mult[5] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f};
        const float *bar = &h.p->a;
        float inc = h.freq * g_drop / SR, s = 0.0f;
        for (int i = 0; i < 5; i++) {
            if (bar[i] > 0.0f) {
                ph[i] += inc * mult[i];
                if (ph[i] > 1.0f) {
                    ph[i] -= 1.0f;
                }
                s += bar[i] * sinf(ph[i] * TWOPI_F);
            }
        }
        float a = amp.Process(h.hold > 0);
        if (h.hold > 0) {
            h.hold--;
        } else if (!amp.IsRunning()) {
            h.on = false;
        }
        return s * a * 0.35f;
    }
};

// ------------------------------------------------------------------ voice ---

struct Vox {
    Head h;
    FormantOscillator f1, f2;
    Svf air;
    Adsr amp;
    float now, lfo;
    void start() {
        const Patch *p = h.p;
        if (p->d > 0.0f) {
            now = h.freq * 0.84f;               // slides up into the note from three semitones under
        } else if (p->glide <= 0.0f || now <= 0.0f) {
            now = h.freq;
        }
        f1.SetFormantFreq(p->a);
        f2.SetFormantFreq(p->b);
        f1.SetPhaseShift(0.0f);
        f2.SetPhaseShift(0.0f);
        air.SetFreq(2800.0f);
        air.SetRes(0.4f);
        set_env(amp, p);
        tune();
    }
    void tune() {
        const Patch *p = h.p;
        float t = p->d > 0.0f ? p->d : p->glide;
        if (t > 0.0f) {
            now += (h.freq - now) * (CTL / (t * SR * 0.35f + CTL));
        }
        lfo += CTL * 5.0f / SR;
        if (lfo > 1.0f) {
            lfo -= 1.0f;
        }
        float fr = now * g_drop * (1.0f + p->vib * 0.05f * sinf(lfo * TWOPI_F));
        f1.SetCarrierFreq(fr);
        f2.SetCarrierFreq(fr);
    }
    float run() {
        const Patch *p = h.p;
        float s = f1.Process() + 0.55f * f2.Process();
        if (p->c > 0.0f) {
            air.Process(frand());
            s = s * (1.0f - p->c * 0.5f) + air.Band() * p->c * 1.5f;
        }
        float a = amp.Process(h.hold > 0);
        if (h.hold > 0) {
            h.hold--;
        } else if (!amp.IsRunning()) {
            h.on = false;
            now = h.freq;
        }
        return s * a * 0.6f;
    }
};

// ------------------------------------------------------------ pitched 808 ---

// The drum model sounds three semitones above the pitch it is given
// (measured with firmware/tools/render.cpp), so it is asked for lower.
static const float kBoomTune = 0.8409f;

struct Boom {
    Head h;
    AnalogBassDrum bd;
    float now, fade;
    int32_t life;
    void start() {
        const Patch *p = h.p;
        if (p->glide <= 0.0f || now <= 0.0f || life <= 0) {
            now = h.freq;
        }
        bd.SetDecay(p->a);
        bd.SetTone(fclamp(p->b * g_tone[h.seat], 0.0f, 1.0f));
        bd.SetAttackFmAmount(p->c);
        bd.SetSelfFmAmount(0.2f);
        bd.SetAccent(0.4f + 0.6f * h.vel);
        bd.SetFreq(now * g_drop * kBoomTune);
        bd.Trig();
        life = (int32_t)(p->d * SR);
        fade = 1.0f;
    }
    void tune() {
        if (h.p->glide > 0.0f) {
            now += (h.freq - now) * (CTL / (h.p->glide * SR * 0.35f + CTL));
        }
        bd.SetFreq(now * g_drop * kBoomTune);
    }
    float run() {
        float s = bd.Process();
        if (--life < 0) {
            fade -= 1.0f / (0.03f * SR);
            if (fade <= 0.0f) {
                h.on = false;
                return 0.0f;
            }
        }
        return s * fade;
    }
};

// ----------------------------------------------------------------- sample ---

struct Tape {
    Head h;
    Adsr amp;
    const pb_sample_t *smp;
    float pos, inc;
    void start() {
        const Patch *p = h.p;
        smp = &pb_samples[(int)p->a];
        pos = 0.0f;
        set_env(amp, p);
        tune();
    }
    void tune() {
        const Patch *p = h.p;
        float r = (float)SAMPLE_RATE_HZ / SR * g_drop;
        if (p->b > 0.0f) {                      // played at the pitch asked for
            r *= h.freq / mtof(p->b);
        } else {
            r *= h.freq;                        // freq carries a plain speed for unpitched recordings
        }
        inc = r;
    }
    float run() {
        uint32_t i = (uint32_t)pos;
        if (i + 1 >= smp->len) {
            if (h.p->c > 0.0f) {
                pos -= (float)(smp->len - 1);
                i = (uint32_t)pos;
            } else {
                h.on = false;
                return 0.0f;
            }
        }
        float fr = pos - (float)i;
        float s = (smp->data[i] * (1.0f - fr) + smp->data[i + 1] * fr) * (1.0f / 32768.0f);
        pos += inc;
        float a = amp.Process(h.hold > 0);
        if (h.hold > 0) {
            h.hold--;
        } else if (!amp.IsRunning()) {
            h.on = false;
        }
        return s * a;
    }
};

// ------------------------------------------------------------------ noise ---

struct Hiss {
    Head h;
    Svf f;
    Adsr amp;
    float cut, mul;
    void start() {
        const Patch *p = h.p;
        cut = p->b;
        mul = powf(p->c / p->b, (float)CTL / (float)(h.hold > 0 ? h.hold : 1));
        f.SetRes(p->res);
        f.SetDrive(0.0f);
        f.SetFreq(cut);
        set_env(amp, p);
        amp.SetAttackTime(p->A < 0.9f ? p->A : h.hold / SR);    // a riser climbs for as long as it is held
    }
    void tune() {
        if (mul != 1.0f && h.hold > 0) {
            cut *= mul;
            f.SetFreq(fclamp(cut * g_drop, 30.0f, 7000.0f));
        }
    }
    float run() {
        f.Process(frand());
        int mode = (int)h.p->a;
        float s = mode == 0 ? f.Low() * 2.0f : mode == 1 ? f.High() : f.Band();
        float a = amp.Process(h.hold > 0);
        if (h.hold > 0) {
            h.hold--;
        } else if (!amp.IsRunning()) {
            h.on = false;
        }
        return s * a;
    }
};

// ------------------------------------------------------------------ pools ---

static Analog g_analog[8];
static Fm g_fm[4];
static Pluck g_string[4];
static Organ g_organ[4];
static Vox g_vox[3];
static Boom g_boom[2];
static Tape g_tape[6];
static Hiss g_hiss[3];

// ------------------------------------------------------------------ drums ---

struct Hit {                        // one drum: who struck it, how loud, how long it rings
    uint8_t seat;
    float lvl, drive, crush, held;
    int32_t left;
    int n;
};
static Hit g_hit[D_COUNT];
static AnalogBassDrum d_boomy, d_tom[3], d_conga;
static SyntheticBassDrum d_punchy;
static AnalogSnareDrum d_snare;
static SyntheticSnareDrum d_rim;
static HiHat<> d_hat, d_open, d_cym;
static Svf d_clap_f, d_shake_f, d_bell_f;
static Oscillator d_bell[2];
static float d_clap_fall, d_clap_env, d_clap_tail, d_shake_env, d_shake_dec, d_bell_env;
static int32_t d_clap_n;
static uint8_t d_kick_kind;

static void strike(int d, int seat, float vel, int kit_no, int note) {
    if (d < 0 || d >= D_COUNT) {
        return;
    }
    const Kit *k = &kKits[kit_no < K_COUNT ? kit_no : 0];
    Hit *h = &g_hit[d];
    float tune = semis((float)note) * g_drop;
    float acc = 0.35f + 0.65f * vel;
    float life = 1.0f;
    h->seat = seat;
    h->lvl = 0.9f;
    h->drive = k->drive;
    h->crush = k->crush;
    switch (d) {
        case D_KICK: case D_LONG_KICK: case D_HARD_KICK: {
            bool punchy = d == D_HARD_KICK || (d == D_KICK && k->kick_synth);
            float dec = d == D_LONG_KICK ? fmaxf(k->kick_dec, 0.8f) : k->kick_dec;
            // all three share one drum, so a new kick cuts the last one off cleanly
            g_hit[D_KICK].left = g_hit[D_LONG_KICK].left = g_hit[D_HARD_KICK].left = 0;
            d_kick_kind = punchy;
            if (punchy) {
                d_punchy.SetFreq(k->kick_f * tune);
                d_punchy.SetDecay(dec);
                d_punchy.SetTone(k->kick_tone);
                d_punchy.SetDirtiness(d == D_HARD_KICK ? 0.6f : 0.2f);
                d_punchy.SetFmEnvelopeAmount(0.6f);
                d_punchy.SetFmEnvelopeDecay(0.3f);
                d_punchy.SetAccent(acc);
                d_punchy.Trig();
                if (d == D_HARD_KICK) {
                    h->drive = fmaxf(h->drive, 0.5f);
                }
            } else {
                d_boomy.SetFreq(k->kick_f * tune);
                d_boomy.SetDecay(dec);
                d_boomy.SetTone(k->kick_tone);
                d_boomy.SetAttackFmAmount(0.5f);
                d_boomy.SetSelfFmAmount(0.3f);
                d_boomy.SetAccent(acc);
                d_boomy.Trig();
            }
            life = 0.5f + dec * 2.5f;
            h->lvl = punchy ? 1.0f : 6.0f;      // the boomy model is much quieter than the rest
            break;
        }
        case D_SNARE:
            d_snare.SetFreq(k->sn_f * tune);
            d_snare.SetDecay(k->sn_dec);
            d_snare.SetSnappy(k->sn_snap);
            d_snare.SetTone(k->sn_tone);
            d_snare.SetAccent(acc);
            d_snare.Trig();
            life = 0.3f + k->sn_dec * 1.5f;
            break;
        case D_RIM:
            d_rim.SetFreq(k->sn_f * 2.0f * tune);
            d_rim.SetDecay(0.12f);
            d_rim.SetSnappy(0.15f);
            d_rim.SetFmAmount(0.2f);
            d_rim.SetAccent(acc);
            d_rim.Trig();
            life = 0.3f;
            h->lvl = 0.8f;
            break;
        case D_CLAP: case D_LONG_CLAP:
            g_hit[D_CLAP].left = g_hit[D_LONG_CLAP].left = 0;
            d_clap_n = 0;
            d_clap_env = acc;
            d_clap_tail = d == D_LONG_CLAP ? fmaxf(k->clap_tail, 0.8f) : k->clap_tail;
            d_clap_f.SetFreq(1100.0f * tune);
            life = d_clap_tail * 4.0f + 0.1f;
            break;
        case D_CLOSED_HAT: case D_PEDAL_HAT:
            g_hit[D_OPEN_HAT].left = 0;         // closing the hat stops the open one
            g_hit[D_CLOSED_HAT].left = g_hit[D_PEDAL_HAT].left = 0;
            d_hat.SetFreq(k->hat_f * tune);
            d_hat.SetDecay(d == D_PEDAL_HAT ? k->hat_dec * 0.5f : k->hat_dec);
            d_hat.SetTone(d == D_PEDAL_HAT ? k->hat_tone * 0.7f : k->hat_tone);
            d_hat.SetNoisiness(k->hat_noise);
            d_hat.SetAccent(acc);
            d_hat.Trig();
            life = 0.4f;
            h->lvl = d == D_PEDAL_HAT ? 2.4f : 1.4f;
            break;
        case D_OPEN_HAT:
            d_open.SetFreq(k->hat_f * tune);
            d_open.SetDecay(fclamp(k->hat_dec + 0.45f, 0.0f, 1.0f));
            d_open.SetTone(k->hat_tone);
            d_open.SetNoisiness(k->hat_noise);
            d_open.SetAccent(acc);
            d_open.Trig();
            life = 1.2f;
            h->lvl = 1.0f;
            break;
        case D_CRASH: case D_RIDE:
            g_hit[D_CRASH].left = g_hit[D_RIDE].left = 0;
            d_cym.SetFreq((d == D_CRASH ? k->hat_f * 0.8f : k->hat_f * 1.25f) * tune);
            d_cym.SetDecay(d == D_CRASH ? 0.95f : 0.8f);
            d_cym.SetTone(d == D_CRASH ? 0.7f : 0.85f);
            d_cym.SetNoisiness(d == D_CRASH ? 0.9f : 0.35f);
            d_cym.SetAccent(acc);
            d_cym.Trig();
            life = d == D_CRASH ? 2.5f : 1.5f;
            h->lvl = 0.5f;
            break;
        case D_LOW_TOM: case D_MID_TOM: case D_HIGH_TOM: {
            static const float up[3] = {1.0f, 1.34f, 1.68f};
            AnalogBassDrum *t = &d_tom[d - D_LOW_TOM];
            t->SetFreq(k->tom_f * up[d - D_LOW_TOM] * tune);
            t->SetDecay(k->tom_dec);
            t->SetTone(0.35f);
            t->SetAttackFmAmount(0.25f);
            t->SetSelfFmAmount(0.1f);
            t->SetAccent(acc);
            t->Trig();
            life = 0.4f + k->tom_dec * 2.0f;
            h->lvl = 2.5f;
            break;
        }
        case D_SHAKER: case D_CLICK:
            g_hit[D_SHAKER].left = g_hit[D_CLICK].left = 0;
            d_shake_env = acc;
            d_shake_dec = d == D_SHAKER ? 0.035f : 0.004f;
            d_shake_f.SetFreq((d == D_SHAKER ? 5200.0f : 2400.0f) * tune);
            life = 0.25f;
            h->lvl = d == D_SHAKER ? 0.5f : 0.8f;
            break;
        case D_COWBELL:
            d_bell[0].SetFreq(540.0f * tune);
            d_bell[1].SetFreq(800.0f * tune);
            d_bell_env = acc;
            life = 0.6f;
            h->lvl = 0.5f;
            break;
        case D_CONGA:
            d_conga.SetFreq(210.0f * tune);
            d_conga.SetDecay(0.22f);
            d_conga.SetTone(0.6f);
            d_conga.SetAttackFmAmount(0.15f);
            d_conga.SetSelfFmAmount(0.05f);
            d_conga.SetAccent(acc);
            d_conga.Trig();
            life = 0.6f;
            break;
        default:
            return;
    }
    h->left = (int32_t)(life * SR);
    h->n = 0;
}

static inline float drum_sample(int d) {
    switch (d) {
        case D_KICK: case D_LONG_KICK: case D_HARD_KICK:
            return d_kick_kind ? d_punchy.Process() : d_boomy.Process();
        case D_SNARE:
            return d_snare.Process();
        case D_RIM:
            return d_rim.Process();
        case D_CLAP: case D_LONG_CLAP: {
            // three quick bursts 11 ms apart, then a tail
            const int32_t burst = PB_SAMPLE_RATE * 11 / 1000;
            int32_t n = d_clap_n++;
            float e;
            if (n < 3 * burst) {
                e = 1.0f - (float)(n % burst) / (float)burst;
            } else {
                if (((n - 3 * burst) & 15) == 0) {     // the tail's fall, worked out now and then
                    d_clap_fall = pb_powf(2.0f, -1.4427f * (float)(n - 3 * burst) / (d_clap_tail * SR * 0.3f));
                }
                e = d_clap_fall;
            }
            d_clap_f.Process(frand());
            return d_clap_f.Band() * e * d_clap_env * 1.6f;
        }
        case D_CLOSED_HAT: case D_PEDAL_HAT:
            return d_hat.Process();
        case D_OPEN_HAT:
            return d_open.Process();
        case D_CRASH: case D_RIDE:
            return d_cym.Process();
        case D_LOW_TOM: case D_MID_TOM: case D_HIGH_TOM:
            return d_tom[d - D_LOW_TOM].Process();
        case D_SHAKER: case D_CLICK:
            d_shake_env *= 1.0f - 1.0f / (d_shake_dec * SR);
            d_shake_f.Process(frand());
            return d_shake_f.High() * d_shake_env;
        case D_COWBELL:
            d_bell_env *= 1.0f - 1.0f / (0.09f * SR);
            d_bell_f.Process(d_bell[0].Process() + d_bell[1].Process());
            return d_bell_f.Band() * d_bell_env * 1.5f;
        case D_CONGA:
            return d_conga.Process();
    }
    return 0.0f;
}

// "Found" and "Click" can also be recordings: which one comes in the note.
static const uint8_t kFound[] = {S_COINS, S_TING, S_WOOD, S_PAPER, S_PING, S_GLASS, S_TICK, S_POP, S_SNAP};
static Patch g_found = {.eng = E_SAMPLE, .gate = 1200, .lvl = 0.9f, .send = 0.1f, .S = 1.0f, .R = 0.05f};

// ------------------------------------------------------------- the engine ---

extern "C" void pb_init(void) {
    fast_tables();
    for (int i = 0; i < PB_SEATS; i++) {
        g_level[i] = 1.0f;
        g_tone[i] = 1.0f;
    }
    memset(g_echo_buf, 0, sizeof(g_echo_buf));
    for (auto &v : g_analog) {
        for (auto &o : v.o) {
            o.Init(SR);
        }
        v.f.Init(SR);
        v.amp.Init(SR);
        v.h.on = false;
        v.now = 0;
    }
    for (auto &v : g_fm) {
        v.fm.Init(SR);
        v.amp.Init(SR);
        v.h.on = false;
    }
    for (auto &v : g_string) {
        v.h.on = false;
    }
    for (auto &v : g_organ) {
        v.amp.Init(SR);
        v.h.on = false;
    }
    for (auto &v : g_vox) {
        v.f1.Init(SR);
        v.f2.Init(SR);
        v.air.Init(SR);
        v.amp.Init(SR);
        v.h.on = false;
        v.now = 0;
    }
    for (auto &v : g_boom) {
        v.bd.Init(SR);
        v.h.on = false;
        v.now = 0;
        v.life = 0;
    }
    for (auto &v : g_tape) {
        v.amp.Init(SR);
        v.h.on = false;
    }
    for (auto &v : g_hiss) {
        v.f.Init(SR);
        v.amp.Init(SR);
        v.h.on = false;
    }
    d_boomy.Init(SR);
    d_punchy.Init(SR);
    d_conga.Init(SR);
    for (auto &t : d_tom) {
        t.Init(SR);
    }
    d_snare.Init(SR);
    d_rim.Init(SR);
    d_hat.Init(SR);
    d_open.Init(SR);
    d_cym.Init(SR);
    d_clap_f.Init(SR);
    d_clap_f.SetRes(0.55f);
    d_shake_f.Init(SR);
    d_shake_f.SetRes(0.2f);
    d_bell_f.Init(SR);
    d_bell_f.SetFreq(900.0f);
    d_bell_f.SetRes(0.6f);
    for (auto &o : d_bell) {
        o.Init(SR);
        o.SetWaveform(SQR);
        o.SetAmp(0.5f);
    }
    memset(g_hit, 0, sizeof(g_hit));
}

template <typename V, int N>
static void play(V (&pool)[N], const Patch *p, int seat, float freq, float vel, int hold, bool mono) {
    V *v = mono ? &pool[0] : take(pool);
    begin(v->h, p, seat, freq, vel, hold);
    v->start();
}

static void note_on(const pb_cmd_t *c) {
    int seat = c->seat < PB_SEATS ? c->seat : 0;
    float vel = c->vel / 127.0f;
    if (c->sound >= DRUM_BASE) {
        int d = c->sound - DRUM_BASE;
        if (d == D_FOUND || (d == D_CLICK && c->note > 0)) {
            int which = d == D_FOUND ? c->note : 5 + c->note;
            g_found.a = kFound[which >= 0 && which < (int)sizeof(kFound) ? which : 0];
            play(g_tape, &g_found, seat, 1.0f, vel, 0, false);
        } else {
            strike(d, seat, vel, c->arg, c->note);
        }
        return;
    }
    if (c->sound >= I_COUNT) {
        return;
    }
    const Patch *p = &kPatches[c->sound];
    float freq = mtof((float)c->note + 12.0f * p->oct);
    switch (p->eng) {
        case E_ANALOG:  play(g_analog, p, seat, freq, vel, c->arg, p->glide > 0.0f); break;
        case E_FM:      play(g_fm, p, seat, freq, vel, c->arg, false); break;
        case E_STRING:  play(g_string, p, seat, freq, vel, c->arg, false); break;
        case E_ORGAN:   play(g_organ, p, seat, freq, vel, c->arg, false); break;
        case E_FORMANT: play(g_vox, p, seat, freq, vel, c->arg, p->glide > 0.0f || p->d > 0.0f); break;
        case E_BOOM:    play(g_boom, p, seat, freq, vel, c->arg, p->glide > 0.0f); break;
        case E_SAMPLE:  play(g_tape, p, seat, p->b > 0.0f ? freq : 1.0f, vel, c->arg, false); break;
        case E_NOISE:   play(g_hiss, p, seat, freq, vel, c->arg, false); break;
    }
}

extern "C" void pb_command(const pb_cmd_t *c) {
    switch (c->op) {
        case PB_NOTE:
            note_on(c);
            break;
        case PB_SEAT_LEVEL:
            if (c->seat < PB_SEATS) {
                g_level[c->seat] = c->vel / 127.0f * 1.27f;
            }
            break;
        case PB_SEAT_TONE:
            if (c->seat < PB_SEATS) {
                g_tone[c->seat] = powf(2.0f, (c->vel - 64) / 32.0f);
            }
            break;
        case PB_MASTER:
            g_master = c->vel / 127.0f;
            break;
        case PB_DROP:
            g_drop = semis(-(float)c->note / 16.0f);
            break;
        case PB_ECHO:
            g_echo = c->vel / 127.0f;
            if (c->arg) {
                int n = (int)((float)c->arg * (SR / 1000.0f));
                g_echo_len = n < 400 ? 400 : n > ECHO_MAX ? ECHO_MAX : n;
            }
            break;
        case PB_ALL_OFF:
            #define HUSH(pool) for (auto &v : pool) { v.h.hold = 0; }
            HUSH(g_analog) HUSH(g_fm) HUSH(g_organ) HUSH(g_vox) HUSH(g_tape) HUSH(g_hiss)
            for (auto &v : g_string) { v.life = 0; }
            for (auto &v : g_boom) { v.life = 0; }
            break;
    }
}

template <typename V, int N>
static inline void mix(V (&pool)[N], bool ctl, float &dry, float &wet) {
    for (int i = 0; i < N; i++) {
        V &v = pool[i];
        if (!v.h.on) {
            continue;
        }
        if (ctl) {
            v.tune();
        }
        float s = v.run() * v.h.p->lvl * (0.3f + 0.7f * v.h.vel) * g_level[v.h.seat];
        dry += s;
        wet += s * v.h.p->send;
    }
}

extern "C" void pb_render(int16_t *out, int frames) {
    for (int n = 0; n < frames; n++) {
        bool ctl = (g_tick++ % CTL) == 0;
        float dry = 0.0f, wet = 0.0f;
        mix(g_analog, ctl, dry, wet);
        mix(g_fm, ctl, dry, wet);
        mix(g_string, ctl, dry, wet);
        mix(g_organ, ctl, dry, wet);
        mix(g_vox, ctl, dry, wet);
        mix(g_boom, ctl, dry, wet);
        mix(g_tape, ctl, dry, wet);
        mix(g_hiss, ctl, dry, wet);
        for (int d = 0; d < D_COUNT; d++) {
            Hit *h = &g_hit[d];
            if (h->left <= 0) {
                continue;
            }
            float s = drum_sample(d);
            if (h->drive > 0.0f) {
                s = SoftClip(s * (1.0f + h->drive * 4.0f));
            }
            if (h->crush > 0.0f) {
                if (++h->n >= 1 + (int)(h->crush * 6.0f)) {
                    h->n = 0;
                    h->held = s;
                }
                s = h->held;
            }
            if (h->left < 256) {                // a short fade so nothing stops with a click
                s *= h->left * (1.0f / 256.0f);
            }
            h->left--;
            dry += s * h->lvl * g_level[h->seat];
        }
        // the echo: one tap with feedback
        float back = g_echo_buf[g_echo_at] * (1.0f / 32768.0f);
        float in = SoftClip(wet * g_echo + back * 0.38f);
        g_echo_buf[g_echo_at] = (int16_t)(in * 32000.0f);
        if (++g_echo_at >= g_echo_len) {
            g_echo_at = 0;
        }
        float s = SoftClip((dry + back) * g_master * 0.6f);
        out[n] = (int16_t)(s * 32000.0f);
    }
}

// Too much is sounding for the processor: let the oldest note go. Returns 0
// if nothing was left to let go.
extern "C" int pb_shed(void) {
    Head *old = nullptr;
    int32_t *life = nullptr;
    #define OLDEST(pool, lifeptr) for (auto &v : pool) { \
        if (v.h.on && (!old || v.h.born < old->born)) { old = &v.h; life = lifeptr; } }
    OLDEST(g_analog, nullptr) OLDEST(g_fm, nullptr) OLDEST(g_organ, nullptr) OLDEST(g_vox, nullptr)
    OLDEST(g_tape, nullptr) OLDEST(g_hiss, nullptr) OLDEST(g_string, &v.life) OLDEST(g_boom, &v.life)
    if (!old) {
        return 0;
    }
    old->hold = 0;
    old->on = life != nullptr;          // strings and booms fade over 30 ms; the rest stop now
    if (life) {
        *life = 0;
    }
    return 1;
}

extern "C" int pb_active(void) {
    int n = 0;
    #define COUNT(pool) for (auto &v : pool) { n += v.h.on; }
    COUNT(g_analog) COUNT(g_fm) COUNT(g_string) COUNT(g_organ) COUNT(g_vox)
    COUNT(g_boom) COUNT(g_tape) COUNT(g_hiss)
    for (int d = 0; d < D_COUNT; d++) {
        n += g_hit[d].left > 0;
    }
    return n;
}

extern "C" const char *pb_sound_engine(int sound) {
    return sound >= 0 && sound < I_COUNT ? kEngineNames[kPatches[sound].eng] : "drum";
}
