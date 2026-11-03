/* SPDX-License-Identifier: GPL-3.0-only */
/* What every toy shares (Playbox's settings.py, sound.py's scales and tune.py): scale, chords,
 * tempo, and what the toys tell each other. */
#include "wb.h"

const char *const ROLE_NAME[NROLES] = {"", "drums", "bass", "chords", "lead", "voice", "texture", "conductor"};

const char *const SCALE_NAME[NSCALES] = {"Major", "Minor", "Penta", "Blues", "Dorian", "Mixo",
                                         "Phryg", "Harmon", "Pent m"};
const uint8_t DARK_SCALES[NDARK] = {1, 6, 7, 4, 8};
/* Semitones of each scale's seven steps; five- and six-note scales are spread over seven, so that
 * seven steps up is an octave and two and four steps up make a chord in every scale. */
const int8_t SCALE_SEMIS[NSCALES][7] = {
    {0, 2, 4, 5, 7, 9, 11}, {0, 2, 3, 5, 7, 8, 10}, {0, 2, 4, 4, 7, 9, 9},  {0, 3, 5, 6, 7, 10, 10},
    {0, 2, 3, 5, 7, 9, 10}, {0, 2, 4, 5, 7, 9, 10}, {0, 1, 3, 5, 7, 8, 10}, {0, 2, 3, 5, 7, 8, 11},
    {0, 3, 5, 5, 7, 10, 10},
};

const progression_t PROGRESSIONS[NPROG] = {
    {"Pop", 4, {0, 4, 5, 3}},     {"Fifties", 4, {0, 5, 3, 4}}, {"Sad", 4, {5, 3, 0, 4}},
    {"Rock", 4, {0, 3, 4, 3}},    {"Jazz", 4, {1, 4, 0, 5}},    {"Canon", 8, {0, 4, 5, 2, 3, 0, 3, 4}},
    {"Blues", 12, {0, 0, 0, 0, 3, 3, 0, 0, 4, 3, 0, 4}},        {"Royal", 4, {3, 4, 2, 5}},
    {"Epic", 4, {5, 4, 3, 4}},    {"Rise", 4, {0, 3, 5, 4}},    {"Anthem", 4, {0, 5, 2, 6}},
    {"Sway", 2, {0, 3}},          {"Witch", 2, {0, 1}},         {"Dusk", 4, {0, 6, 5, 6}},
    {"Lift", 4, {5, 6, 0, 0}},
};
static const char *const RATE_NAMES[3] = {"1 bar", "2 bars", "4 bars"};
static const int RATES[3] = {1, 2, 4};

settings_t S;

void param_init(param_t *p, const char *name, int lo, int hi, int value, int step,
                const char *const *choices, const char *fmt, const char *help)
{
    p->name = name;
    p->lo = (int16_t)lo;
    p->hi = (int16_t)hi;
    p->value = (int16_t)value;
    p->step = (int16_t)(step ? step : 1);
    p->choices = choices;
    p->fmt = fmt ? fmt : "%d";
    p->help = help ? help : "";
}

int param_turn(param_t *p, int detents)
{
    int v = p->value + detents * p->step;
    if (p->choices) {
        int n = p->hi - p->lo + 1;
        v = p->lo + (((v - p->lo) % n) + n) % n;
    } else {
        v = v < p->lo ? p->lo : v > p->hi ? p->hi : v;
    }
    if (v == p->value)
        return 0;
    p->value = (int16_t)v;
    return 1;
}

static void itoa_sw(int v, char *b, int width)
{
    char t[12];
    int n = 0, neg = v < 0;
    unsigned u = (unsigned)(neg ? -v : v);
    do {
        t[n++] = (char)('0' + u % 10u);
        u /= 10u;
    } while (u);
    while (n < width)
        t[n++] = '0';
    if (neg)
        *b++ = '-';
    while (n)
        *b++ = t[--n];
    *b = 0;
}

void param_text(const param_t *p, char *b)
{
    if (p->choices) {
        const char *s = p->choices[p->value - p->lo];
        int i;
        for (i = 0; s[i] && i < 11; i++)
            b[i] = s[i];
        b[i] = 0;
        return;
    }
    itoa_sw(p->value, b, p->fmt[1] == '0' ? p->fmt[2] - '0' : 0);
    if (p->fmt[2] == '%' || (p->fmt[1] == 'd' && p->fmt[2] == '%')) {
        int i = 0;
        while (b[i])
            i++;
        b[i] = '%';
        b[i + 1] = 0;
    }
}

int param_frac16(const param_t *p)
{
    if (p->hi == p->lo)
        return 0;
    return (p->value - p->lo) * 65536 / (p->hi - p->lo);
}

void settings_init(void)
{
    int i;
    param_init(&S.scale, "Scale", 0, NSCALES - 1, 0, 1, SCALE_NAME, "%d", "The set of notes the toy picks from");
    param_init(&S.tempo, "Tempo", 40, 240, 120, 2, 0, "%d", "Speed, in beats per minute");
    param_init(&S.volume, "Volume", 0, 100, 80, 5, 0, "%d%%", "How loud this toy is");
    param_init(&S.rate, "Rate", 0, 2, 0, 1, RATE_NAMES, "%d", "How long the band stays on each chord");
    S.chords = 1;
    S.rolled[0] = 0;
    S.rolled[1] = 4;
    S.rolled[2] = 5;
    S.rolled[3] = 3;
    S.band = 0;
    S.band_steps = 0;
    S.kicks = S.leads = S.lead = S.has_lead = 0;
    S.drummer = 0;
    S.root_hold = -1;
    S.tone = 50;
    for (i = 0; i < SEATS; i++)
        S.roles[i] = ROLE_NONE;
    settings_calm();
}

void settings_calm(void)
{
    int i;
    S.speed = 100;
    S.drop = 0;
    S.tone = 50;
    for (i = 0; i < SEATS; i++) {
        S.cuts[i] = 0;
        S.levels[i] = 100;
        S.throw_[i] = 0;
    }
}

int chord_bar(int steps)
{
    if (S.band)
        return S.band_steps / (BAR * RATES[S.rate.value]);
    return steps / BAR;
}

void chords_roll(void)
{
    S.rolled[0] = 0;
    S.rolled[1] = (uint8_t)dice_pick(7);
    S.rolled[2] = (uint8_t)dice_pick(7);
    S.rolled[3] = (uint8_t)dice_pick(7);
}

void chords_randomise(void)
{
    S.chords = dice_pick(NPROG + 1);
    chords_roll();
}

int chords_root(int steps, int ahead)
{
    int b = chord_bar(steps) + ahead;
    if (S.root_hold >= 0)
        return S.root_hold;
    if (S.chords == 0)
        return S.rolled[((b % 4) + 4) % 4];
    {
        const progression_t *p = &PROGRESSIONS[S.chords - 1];
        return p->root[((b % p->n) + p->n) % p->n];
    }
}

const char *chords_text(char *b)
{
    const char *s = S.chords == 0 ? "Random" : PROGRESSIONS[S.chords - 1].name;
    int i;
    for (i = 0; s[i] && i < 11; i++)
        b[i] = s[i];
    b[i] = 0;
    return b;
}

int scale_semis(int degree)
{
    int oct = degree >= 0 ? degree / 7 : -((-degree + 6) / 7);     /* Python's floor division */
    int d = degree - oct * 7;
    return 12 * oct + SCALE_SEMIS[S.scale.value][d];
}

int tune_nearest(int note, int root)
{
    static const int8_t D[5] = {0, -1, 1, -2, 2};
    int i;
    for (i = 0; i < 5; i++) {
        int m = (((note + D[i] - root) % 7) + 7) % 7;
        if (m == 0 || m == 2 || m == 4)
            return note + D[i];
    }
    return note;
}

int tune_tones(int root, int lo, int hi, int8_t *out)
{
    int n, c = 0;
    for (n = lo; n <= hi; n++) {
        int m = (((n - root) % 7) + 7) % 7;
        if (m == 0 || m == 2 || m == 4)
            out[c++] = (int8_t)n;
    }
    return c;
}
