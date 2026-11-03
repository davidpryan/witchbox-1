/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 in fm1-base's host simulator (base/host/plat_host.c, which includes app.c and so this):
 * witchbox1's checks for "expect", its commands and a reboot.
 *
 * Checks: playing page seat toy seed knobN scale chords tempo speed steps notes mute seated leds dirty slotN
 * browse hold pick (fm1-base's own: store_writes midi_out peak_db_max peak_db_min cpu). Commands: seat N TOY SEED (a toy
 * into seat N, by its module name, with that seed), foreign (another firmware's project left in the flash object). */

int app_expect(const char *what, int *got)
{
    int g;
    if (!strcmp(what, "playing"))
        g = band_playing;
    else if (!strcmp(what, "page"))
        g = ui.page;
    else if (!strcmp(what, "seat"))
        g = proj.cur;
    else if (!strcmp(what, "toy"))                /* the toy in the seat on screen, or -1 */
        g = cur() ? cur()->id : -1;
    else if (!strcmp(what, "seed"))
        g = cur() ? cur()->seed : -1;
    else if (!strncmp(what, "knob", 4))           /* knobN: knob N of the seat on screen */
        g = cur() ? cur()->p[atoi(what + 4) & 3].value : -9999;
    else if (!strcmp(what, "scale"))
        g = S.scale.value;
    else if (!strcmp(what, "chords"))
        g = S.chords;
    else if (!strcmp(what, "tempo"))
        g = S.tempo.value;
    else if (!strcmp(what, "speed"))
        g = S.speed;
    else if (!strcmp(what, "steps"))
        g = S.band_steps;
    else if (!strcmp(what, "notes"))              /* notes sounded since boot */
        g = (int)note_log_n;
    else if (!strcmp(what, "mute"))
        g = proj.now.seat[proj.cur].mute;
    else if (!strcmp(what, "seated")) {           /* how many seats hold a toy */
        int i;
        g = 0;
        for (i = 0; i < SEATS; i++)
            g += band_seated(i);
    } else if (!strcmp(what, "leds"))
        g = proj.leds;
    else if (!strcmp(what, "dirty"))
        g = ui.dirty;
    else if (!strncmp(what, "slot", 4))           /* slotN: the toy saved in slot N, or -1 */
        g = proj.slot[atoi(what + 4) & 15].toy;
    else if (!strcmp(what, "browse"))
        g = ui.browse;
    else if (!strcmp(what, "hold"))
        g = ui.hold;
    else if (!strcmp(what, "pick"))
        g = ui.pick;
    else
        return 0;
    *got = g;
    return 1;
}

int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    (void)d;
    if (!strcmp(cmd, "seat")) {                   /* seat N TOY SEED */
        int id = toy_find(b);
        if (id < 0) {
            printf("no toy %s\n", b);
            exit(2);
        }
        band_seat(atoi(a), id, atoi(c));
        return 1;
    }
    if (!strcmp(cmd, "foreign")) {                /* a project another firmware left in the flash object */
        static uint8_t o[1200];
        memset(o, 0x55, sizeof o);
        memcpy(o, "ENO1", 4);
        o[4] = 4;
        o[5] = o[6] = o[7] = 0;
        plat_store_save(OBJ_PROJ, o, sizeof o);
        return 1;
    }
    return 0;
}

void app_host_reboot(void) { memset(&proj, 0, sizeof proj); }
