/* SPDX-License-Identifier: GPL-3.0-only */
/* The port, on the host: run one toy and print every note, in run_ref.py's format, so the two can
 * be diffed.   run_c MODULE SEED [--ms N] [--tick MS] [--shake SEED2] [--knobs a,b,c]
 *                [--scale N] [--chords N] [--tempo N] [--params] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/src/app/wb/dice.c"
#include "../firmware/src/app/wb/settings.c"
#include "../firmware/src/app/wb/toy.c"
#include "../firmware/src/app/wb/toys/all.c"

static int now;
void heard(int role, int degree, int voice, int module, int seat, int hold)
{
    (void)module; (void)seat;
    printf("T\t%d\t%s\t%d\t%d\t%d\n", now, ROLE_NAME[role], degree, voice, hold);
    if (role == ROLE_DRUMS && voice == 0)
        S.kicks++;
    else if (role == ROLE_LEAD) {
        S.lead = degree;
        S.has_lead = 1;
        S.leads++;
    }
}

static int l_speed = -1, l_drop = -1, l_cuts = -1, l_lev[4] = {-1, -1, -1, -1};
static void watch(void)
{
    int i, cuts = 0, lev = 0;
    if (S.speed != l_speed) { l_speed = S.speed; printf("S\t%d\tspeed\t%d\n", now, S.speed); }
    if (S.drop != l_drop) { l_drop = S.drop; printf("S\t%d\tdrop\t%d\n", now, S.drop); }
    for (i = 0; i < 4; i++) cuts |= S.cuts[i] << i;
    if (cuts != l_cuts) {
        l_cuts = cuts;
        printf("S\t%d\tcuts\t", now);
        for (i = 0; i < 4; i++) printf("%s%s", i ? "," : "", S.cuts[i] ? "True" : "False");
        printf("\n");
    }
    for (i = 0; i < 4; i++) lev |= S.levels[i] != l_lev[i];
    if (lev) {
        printf("S\t%d\tlevels\t", now);
        for (i = 0; i < 4; i++) { l_lev[i] = S.levels[i]; printf("%s%d", i ? "," : "", S.levels[i]); }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    static toy_t t;
    int ms = 20000, tick = 5, shake = -1, params = 0, scale = 1, chords = 1, tempo = 120, i, id;
    int16_t knobs[MAX_PARAMS]; int nk = 0;
    if (argc < 3) { fprintf(stderr, "run_c MODULE SEED ...\n"); return 2; }
    for (i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--ms")) ms = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--tick")) tick = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--shake")) shake = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--scale")) scale = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--chords")) chords = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--tempo")) tempo = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--params")) params = 1;
        else if (!strcmp(argv[i], "--knobs")) {
            char *p = argv[++i];
            while (*p && nk < MAX_PARAMS) { knobs[nk++] = (int16_t)strtol(p, &p, 10); if (*p == ',') p++; }
        }
    }
    settings_init();
    S.scale.value = (int16_t)scale;
    S.tempo.value = (int16_t)tempo;
    S.chords = chords;
    id = toy_find(argv[1]);
    if (id < 0) { fprintf(stderr, "no such toy: %s\n", argv[1]); return 2; }
    toy_open(&t, id, atoi(argv[2]));
    if (nk) toy_load(&t, knobs);
    if (shake >= 0) toy_shake(&t, shake);
    if (params) {
        for (i = 0; i < t.np; i++) printf("P\t%s\t%d\t%d\t%d\n", t.p[i].name, t.p[i].lo, t.p[i].hi, t.p[i].value);
        printf("P\tScale\t0\t%d\t%d\n", NSCALES - 1, S.scale.value);
        printf("P\tChords\t0\t%d\t%d\n", NPROG, S.chords);
        printf("P\tRolled\t%d\t%d\t%d\n", S.rolled[1], S.rolled[2], S.rolled[3]);
    }
    toy_play(&t, 1);
    for (now = 0; now < ms;) {
        toy_tick(&t, tick);
        now += tick;
        watch();
    }
    return 0;
}
