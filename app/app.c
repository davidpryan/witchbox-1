/* SPDX-License-Identifier: GPL-3.0-only */
/* witchbox1 on fm1-base: the app's unity build (compiled into fm1-base's, after the platform) and the glue
 * between fm1-base's contract (app.h) and witchbox1's parts: the toys and their dice, settings and sounds
 * (wb/), the band (band.c), the project (project.c), the UI (ui.c) and the engine (dsp/wb_engine.c, its own
 * -O2 unit). */
#include "app.h"
#define WB_PICTURES                      /* the toys draw their pictures (wb/toys/pic.h) */
#include "wb/dice.c"
#include "wb/settings.c"
#include "wb/toy.c"
#include "wb/toys/all.c"
#include "wb/sound.c"
#include "band.c"
#include "project.c"
#include "ui.c"

/* the engine's MIDI out: to USB */
void en_midi_out(uint32_t st, uint32_t d1, uint32_t d2) { plat_midi_out(st >> 4 | st << 8 | d1 << 16 | d2 << 24); }

/* ---- the contract */
void app_init(void)
{
    en_init();
    band_init();
    project_load();                      /* defaults when nothing (or another firmware's project) is saved */
    project_apply();
}
void app_render(int32_t *lr, uint32_t n, uint32_t gain_q12) { en_render(lr, n, gain_q12); }
void app_panic(void)
{
    band_play(0);
    en_panic();
}
void app_overload(void) { en_shed(); }
void app_ui_init(void) { ui_init(); }
void app_frame(void) { ui_frame(); }
void app_input_only(void) { ui_input_only(); }
void app_say(const char *a, const char *b) { ui_say(a, b); }

/* ---- backup and restore over USB (fm1.toml backup = true) */
int app_backup_playing(void) { return band_playing; }
void app_backup_flush(void)
{
    if (ui_dirty())
        project_save();
}
void app_backup_hold(int on) { ui_hold(on); }
void app_backup_restored(void)
{
    band_play(0);
    project_load();
    project_apply();
    ui_init();
}

#ifdef PLAT_HOST
#include "host.c"                        /* the host simulator's part: checks and commands */
#endif
