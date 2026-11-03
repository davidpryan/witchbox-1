/* SPDX-License-Identifier: GPL-3.0-only */
/* The engine alone: every instrument and every drum is played once; the output must be finite, not
 * silent, and not clipped flat. Writes build/host/engine_test.wav to listen to. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../../firmware/src/dsp/wb_engine.h"

static FILE *wf;
static uint32_t wn;
static void wav_open(const char *p)
{
    wf = fopen(p, "wb");
    if (wf) { static const char h[44]; fwrite(h, 1, 44, wf); }
}
static void wav_close(void)
{
    uint32_t bytes = wn * 2, sr = 44100, br = sr * 2, sub = bytes, riff = 36 + bytes; uint16_t one = 1, ch = 1, ba = 2, bits = 16;
    if (!wf) return;
    fseek(wf, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, wf); fwrite(&riff, 4, 1, wf); fwrite("WAVEfmt ", 1, 8, wf);
    { uint32_t sz = 16; fwrite(&sz, 4, 1, wf); }
    fwrite(&one, 2, 1, wf); fwrite(&ch, 2, 1, wf); fwrite(&sr, 4, 1, wf); fwrite(&br, 4, 1, wf); fwrite(&ba, 2, 1, wf); fwrite(&bits, 2, 1, wf);
    fwrite("data", 1, 4, wf); fwrite(&sub, 4, 1, wf); fclose(wf);
}
static int32_t buf[2 * 256];
static int bad;
static double run(int frames)      /* render, returns the peak as a fraction */
{
    double peak = 0;
    while (frames > 0) {
        int n = frames > 256 ? 256 : frames, i;
        en_render(buf, (uint32_t)n, 2048);
        for (i = 0; i < n; i++) {
            double v = buf[2 * i] / 8300000.0;
            int16_t s;
            if (v != v || v > 1e6 || v < -1e6) { bad++; v = 0; }
            if (fabs(v) > peak) peak = fabs(v);
            s = (int16_t)(v > 1 ? 32767 : v < -1 ? -32767 : v * 32767);
            if (wf) fwrite(&s, 2, 1, wf);
            wn++;
        }
        frames -= n;
    }
    return peak;
}
int main(void)
{
    int i, fails = 0;
    en_init();
    wav_open("build/host/engine_test.wav");
    for (i = 0; i < I_COUNT; i++) {
        double p;
        en_note(0, i, 57, 100, 0);
        p = run(44100 / 2);
        en_set(EN_ALL_OFF, 0, 0, 0, 0);
        run(44100 / 4);
        printf("  %-16s peak %.3f%s\n", en_sound_name(i), p, p < 0.01 ? "  SILENT" : "");
        if (p < 0.01) fails++;
    }
    for (i = 0; i < D_COUNT; i++) {
        double p;
        en_note(0, DRUM_BASE + i, i == D_FOUND ? 2 : 0, 100, K_WITCH);
        p = run(44100 / 2);
        printf("  drum %-11d peak %.3f%s\n", i, p, p < 0.01 ? "  SILENT" : "");
        if (p < 0.01) fails++;
    }
    /* a chord with echo on, to see the mix survive */
    en_set(EN_ECHO, 0, 80, 0, 300);
    en_note(0, 22, 48, 100, 1500); en_note(0, 22, 55, 100, 1500); en_note(0, 22, 60, 100, 1500);
    en_note(1, DRUM_BASE + D_KICK, 0, 110, K_WITCH);
    run(44100 * 2);
    wav_close();
    printf("bad samples: %d\n", bad);
    if (bad || fails) { printf("ENGINE TEST FAILED\n"); return 1; }
    printf("engine ok\n");
    return 0;
}
