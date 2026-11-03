/* SPDX-License-Identifier: GPL-3.0-only */
/* Every Witchbox toy, in shelf order (Playbox's pages three and four). Part of the unity build. */
#include "toy_util.h"
#include "twostep.c"
#include "hatroll.c"
#include "stomp.c"
#include "pocket.c"
#include "limp.c"
#include "undertow.c"
#include "slide808.c"
#include "octpump.c"
#include "reese.c"
#include "drone.c"
#include "pumppad.c"
#include "coven.c"
#include "gate.c"
#include "wetpiano.c"
#include "cathedral.c"
#include "glassarp.c"
#include "bitarp.c"
#include "hook.c"
#include "lost.c"
#include "screamer.c"
#include "fourfloor.c"
#include "rush.c"
#include "clock.c"
#include "toms.c"
#include "riser.c"
#include "rave.c"
#include "swell.c"
#include "tapeloop.c"
#include "crackle.c"
#include "nightbus.c"
#include "musicbox.c"
#include "siren.c"
#include "shadow.c"
#include "ghost.c"
#include "chop.c"
#include "screwed.c"
#include "breath.c"
#include "blackout.c"
#include "screw.c"
#include "arc.c"
#include "heart.c"
#include "chopbreak.c"
#include "scrap.c"
#include "rattle.c"
#include "phonk.c"
#include "bodybass.c"
#include "coldwave.c"
#include "drip.c"
#include "afterhours.c"
#include "vespers.c"
#include "graveyard.c"
#include "shiver.c"
#include "hexbell.c"
#include "cartridge.c"
#include "helium.c"
#include "storm.c"
#include "murder.c"
#include "toll.c"
#include "fog.c"
#include "dubout.c"
#include "turing.c"
#include "ratchet.c"
#include "markovkit.c"
#include "automaton.c"
#include "sleepwalk.c"
#include "pulsar.c"
#include "hysteria.c"
#include "seance.c"
#include "lichen.c"
#include "round.c"
#include "fractal.c"
#include "oracle.c"
#include "loom.c"
#include "golden.c"
#include "haunting.c"
#include "chimes.c"
#include "whisper.c"
#include "static.c"
#include "culling.c"
#include "moon.c"

const toy_class_t *const TOYS[] = {
    &toy_twostep,
    &toy_hatroll,
    &toy_stomp,
    &toy_pocket,
    &toy_limp,
    &toy_undertow,
    &toy_slide808,
    &toy_octpump,
    &toy_reese,
    &toy_drone,
    &toy_pumppad,
    &toy_coven,
    &toy_gate,
    &toy_wetpiano,
    &toy_cathedral,
    &toy_glassarp,
    &toy_bitarp,
    &toy_hook,
    &toy_lost,
    &toy_screamer,
    &toy_fourfloor,
    &toy_rush,
    &toy_clock,
    &toy_toms,
    &toy_riser,
    &toy_rave,
    &toy_swell,
    &toy_tapeloop,
    &toy_crackle,
    &toy_nightbus,
    &toy_musicbox,
    &toy_siren,
    &toy_shadow,
    &toy_ghost,
    &toy_chop,
    &toy_screwed,
    &toy_breath,
    &toy_blackout,
    &toy_screw,
    &toy_arc,
    &toy_heart,
    &toy_chopbreak,
    &toy_scrap,
    &toy_rattle,
    &toy_phonk,
    &toy_bodybass,
    &toy_coldwave,
    &toy_drip,
    &toy_afterhours,
    &toy_vespers,
    &toy_graveyard,
    &toy_shiver,
    &toy_hexbell,
    &toy_cartridge,
    &toy_helium,
    &toy_storm,
    &toy_murder,
    &toy_toll,
    &toy_fog,
    &toy_dubout,
    &toy_turing,
    &toy_ratchet,
    &toy_markovkit,
    &toy_automaton,
    &toy_sleepwalk,
    &toy_pulsar,
    &toy_hysteria,
    &toy_seance,
    &toy_lichen,
    &toy_round,
    &toy_fractal,
    &toy_oracle,
    &toy_loom,
    &toy_golden,
    &toy_haunting,
    &toy_chimes,
    &toy_whisper,
    &toy_static,
    &toy_culling,
    &toy_moon,
};
const int NTOYS = (int)(sizeof TOYS / sizeof TOYS[0]);

/* every toy's state must fit the block a seat holds (wb.h MAX_STATE): a build error here means a
 * toy grew */
typedef union {
    twostep_t a1; hatroll_t a2; stomp_t a3; pocket_t a4; limp_t a5; undertow_t a6; slide808_t a7;
    octpump_t a8; reese_t a9; drone_t a10; pumppad_t a11; coven_t a12; gate_t a13; wetpiano_t a14;
    cathedral_t a15; glassarp_t a16; bitarp_t a17; hook_t a18; lost_t a19; screamer_t a20;
    fourfloor_t a21; rush_t a22; clock_t_ a23; toms_t a24; riser_t a25; rave_t a26; swell_t a27;
    tapeloop_t a28; crackle_t a29; nightbus_t a30; musicbox_t a31; siren_t a32; shadow_t a33;
    ghost_t a34; chop_t a35; screwed_t a36; breath_t a37; blackout_t a38; screw_t a39; arc_t a40;
    heart_t b1; chopbreak_t b2; scrap_t b3; rattle_t b4; phonk_t b5; bodybass_t b6; coldwave_t b7; drip_t b8; afterhours_t b9; vespers_t b10; graveyard_t b11; shiver_t b12; hexbell_t b13; cartridge_t b14; helium_t b15; storm_t b16; murder_t b17; toll_t b18; fog_t b19; dubout_t b20;
    turing_t c1; ratchet_t c2; markovkit_t c3; automaton_t c4; sleepwalk_t c5; pulsar_t c6; hysteria_t c7; seance_t c8; lichen_t c9; round_t c10; fractal_t c11; oracle_t c12; loom_t c13; golden_t c14; haunting_t c15; chimes_t c16; whisper_t c17; static_t c18; culling_t c19; moon_t c20;
} any_state_t;
typedef char state_fits_the_seat[sizeof(any_state_t) <= MAX_STATE ? 1 : -1];
