/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 project: one flash object (OBJ_PROJ) holding the band as it is, sixteen saved toys and
 * four saved bands. A project of another format, or none, gives the defaults. Part of the unity
 * build, after app.h. */
project_t proj;
/* the whole project is one store object: a build error here means it outgrew the flash sector */
typedef char project_fits_the_store[sizeof(project_t) <= PLAT_STORE_MAX ? 1 : -1];

static void rec_of(int i, seat_rec_t *r)        /* seat i as it is now */
{
    const toy_t *t = &band[i];
    int k;
    r->toy = (int8_t)(t->c ? t->id : -1);
    r->mute = proj.now.seat[i].mute;
    r->seed = t->c ? t->seed : 0;
    for (k = 0; k < 4; k++)
        r->knob[k] = t->c && k < t->np ? t->p[k].value : 0;
    r->level = (uint8_t)seat_level[i];
    r->rsv = 1;                     /* the knobs are set (a default record leaves them to the toy) */
}

static void seat_from(int i, const seat_rec_t *r)
{
    band_seat(i, r->toy, r->seed);
    if (r->toy >= 0 && (r->rsv & 1)) {
        int16_t v[MAX_PARAMS] = {0};
        int k;
        for (k = 0; k < 4; k++)
            v[k] = r->knob[k];
        toy_load(&band[i], v);
    }
    proj.now.seat[i].mute = r->mute;
    seat_level[i] = r->level;
}

void project_keep(void)
{
    int i;
    for (i = 0; i < SEATS; i++)
        rec_of(i, &proj.now.seat[i]);
    proj.now.scale = (uint8_t)S.scale.value;
    proj.now.chords = (uint8_t)S.chords;
    proj.now.rate = (uint8_t)S.rate.value;
    proj.now.tempo = (uint8_t)((S.tempo.value - 40) / 2);
    for (i = 0; i < 4; i++)
        proj.now.rolled[i] = S.rolled[i];
    proj.volume = (uint8_t)S.volume.value;
}

static void scene_apply(const scene_t *sc)
{
    int i;
    S.scale.value = (int16_t)(sc->scale < NSCALES ? sc->scale : 1);
    S.chords = sc->chords <= NPROG ? sc->chords : 1;
    S.rate.value = (int16_t)(sc->rate < 3 ? sc->rate : 0);
    S.tempo.value = (int16_t)(40 + (sc->tempo <= 100 ? sc->tempo : 40) * 2);
    for (i = 0; i < 4; i++)
        S.rolled[i] = (uint8_t)(sc->rolled[i] % 7);
    for (i = 0; i < SEATS; i++)
        seat_from(i, &sc->seat[i]);
    for (i = 0; i < 4; i++)
        en_set(EN_FX, 0, sc->fx[i], i, i == 2 ? 0 : 0);
}

void project_apply(void)
{
    scene_apply(&proj.now);
    S.volume.value = (int16_t)(proj.volume <= 100 ? proj.volume : 80);
    sound_resend();
}

void project_defaults(void)
{
    int i;
    memset(&proj, 0, sizeof proj);
    proj.magic = PROJ_MAGIC;
    proj.format = PROJ_FORMAT;
    proj.leds = 1;
    proj.volume = 80;
    for (i = 0; i < NSLOTS; i++)
        proj.slot[i].toy = -1;
    for (i = 0; i < NSCENES; i++) {
        int j;
        for (j = 0; j < SEATS; j++)
            proj.scene[i].seat[j].toy = -1;
    }
    proj.now.scale = 1;
    proj.now.chords = 13;           /* Dusk */
    proj.now.tempo = (130 - 40) / 2;
    for (i = 0; i < SEATS; i++) {
        proj.now.seat[i].toy = -1;
        proj.now.seat[i].level = 80;
    }
    proj.now.seat[0].toy = (int8_t)toy_find("twostep");
    proj.now.seat[0].seed = 1234;
    proj.now.seat[1].toy = (int8_t)toy_find("undertow");
    proj.now.seat[1].seed = 1234;
    proj.now.fx[2] = 64;
    proj.now.fx[3] = 54;
    proj.now.mode = MODE_BAND;
    proj.now.tape_len = 16;
    for (i = 0; i < SEATS; i++) {
        proj.now.dots[i] = 4;
        proj.now.span[i] = 0;
    }
}

static int rec_ok(const seat_rec_t *r)
{
    int k;
    for (k = 0; k < 4; k++)
        if (r->knob[k] < -100 || r->knob[k] > 100)
            return 0;
    return r->toy >= -1 && r->toy < NTOYS && r->seed < 10000 && r->level <= 100;
}

static int scene_ok(const scene_t *s)
{
    int i;
    for (i = 0; i < SEATS; i++)
        if (!rec_ok(&s->seat[i]))
            return 0;
    return s->scale < NSCALES && s->chords <= NPROG && s->rate < 3 && s->tempo <= 100 && s->mode < NMODES && s->tape_len <= TAPE_MAX;
}

static int project_valid(void)
{
    int i;
    if (proj.magic != PROJ_MAGIC || proj.format != PROJ_FORMAT || proj.cur >= SEATS)
        return 0;
    if (!scene_ok(&proj.now))
        return 0;
    for (i = 0; i < NSLOTS; i++)
        if (!rec_ok(&proj.slot[i]))
            return 0;
    for (i = 0; i < NSCENES; i++)
        if (!scene_ok(&proj.scene[i]))
            return 0;
    return 1;
}

int project_load(void)
{
    int n = plat_store_load(OBJ_PROJ, &proj, sizeof proj);
    if (n == (int)sizeof proj && project_valid())
        return 0;
    project_defaults();
    return -1;
}

int project_save(void)
{
    project_keep();
    return plat_store_save(OBJ_PROJ, &proj, sizeof proj);
}

void project_save_slot(int i)           /* the seat on screen into toy slot i */
{
    if (i >= 0 && i < NSLOTS)
        rec_of(proj.cur, &proj.slot[i]);
}

int project_load_slot(int i)             /* toy slot i into the seat on screen; 0 = done */
{
    if (i < 0 || i >= NSLOTS || proj.slot[i].toy < 0)
        return -1;
    seat_from(proj.cur, &proj.slot[i]);
    return 0;
}

void project_save_scene(int i)
{
    if (i >= 0 && i < NSCENES) {
        project_keep();
        proj.scene[i] = proj.now;
    }
}

int project_load_scene(int i)
{
    int j, any = 0;
    if (i < 0 || i >= NSCENES)
        return -1;
    for (j = 0; j < SEATS; j++)
        any |= proj.scene[i].seat[j].toy >= 0;
    if (!any)
        return -1;
    proj.now = proj.scene[i];
    scene_apply(&proj.now);
    return 0;
}
