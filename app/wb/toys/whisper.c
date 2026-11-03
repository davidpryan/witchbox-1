/* SPDX-License-Identifier: GPL-3.0-only */
/* Toy 117 (Witchbox IV, texture): Whisper. Breathy voice fragments, quiet and far between, that
 * come in bursts: a few close together, then nothing for a while (a Poisson burst process). */
#include "toy_util.h"
#undef F
#define F(n) wh_##n

typedef struct { int burst, wait; } whisper_t;
#define DENSITY 0
#define BURSTS 1
#define LENGTH 2
#define DOUBLES 3

static void F(make)(toy_t *t)
{
    add(t, "Density", 0, 10, 3, "How often a burst of whispers starts");
    add(t, "Bursts", 1, 6, 3, "How many whispers in a burst");
    add(t, "Length", 1, 4, 2, "How long each whisper is, in steps");
    add(t, "Doubles", 0, 10, 3, "How often a second voice whispers too");
}

static void F(randomise)(toy_t *t)
{
    P(DENSITY) = (int16_t)dice_int(11);
    P(BURSTS) = (int16_t)(1 + dice_int(6));
    P(LENGTH) = (int16_t)(1 + dice_int(4));
    P(DOUBLES) = (int16_t)dice_int(11);
}

static void F(play_changed)(toy_t *t) { ST(whisper_t)->burst = ST(whisper_t)->wait = 0; }

static void F(on_step)(toy_t *t)
{
    whisper_t *s = ST(whisper_t);
    int root = toy_root(t, 0), deg, hold = P(LENGTH) * t->step_ms - 30;
    if (s->wait > 0) {
        s->wait--;
        return;
    }
    if (s->burst <= 0) {
        if (!dice_lt(P(DENSITY), 50))
            return;
        s->burst = 1 + dice_int(P(BURSTS));
    }
    s->burst--;
    s->wait = P(LENGTH) + dice_int(3);
    deg = root + dice_int(7) - 3;
    toy_note(t, deg, 0, hold);
    if (dice_lt(P(DOUBLES), 11))
        toy_note(t, tune_nearest(deg + 2 + dice_int(3), root), 1, hold);
}

#ifdef WB_PICTURES
/* the picture: a seal with the voice rune at its heart; each whisper a ring that opens on the seal
 * where it landed and widens as it fades, in the hue while it breathes and sand after, a second
 * voice soft; a burst leaves several rings opening at once */
static void F(draw)(toy_t *t)
{
    whisper_t *s = ST(whisper_t);
    uint32_t now = plat_ms(), i;
    int cx = PIC_CX, cy = PIC_CY, k;
    pic_seal(cx, cy, 64, 6, 0, K_SAND);
    for (k = 0; k < 6; k++)
        pic_pt(cx, cy, 58, k, 6, 0, K_SAND);
    for (i = 0; i < NOTE_LOG; i++) {
        const note_log_t *l = &note_log[i];
        uint32_t age;
        int a, r, x, y;
        if (!l->at || l->seat != (uint8_t)t->seat || now - l->at > 3000u)
            continue;
        age = now - l->at;
        a = (int)(pic_hash(l->at, 3) & 31u);
        r = 20 + (int)(pic_hash(l->at, 7) % 24u);
        x = pic_px(cx, r, a);
        y = pic_py(cy, r, a);
        pic_circle(x, y, 3 + (int)(age * 14u / 3000u), age < 600u ? (l->voice ? K_SOFT : pic_hue) : K_SAND);
        if (age < 600u)
            pic_disc(x, y, 2, l->voice ? K_SOFT : pic_hue);
    }
    pic_disc(cx, cy, 10, K_BG);
    pic_rune(cx - 4, cy - 6, RUNE_VOICE, s->burst > 0 ? pic_hue : K_TEXT);
}
#endif

TOY(toy_whisper, "whisper", "Whisper", "whisper", ROLE_TEXTURE, HUE_PINK, 0, 3, "Breathy voice fragments, quiet and far between, that come in bursts and then fall silent.", whisper_t, F(make), 0, 0, F(randomise), F(on_step), F(play_changed), 0, PIC(F(draw)), 0);
#undef DENSITY
#undef BURSTS
#undef LENGTH
#undef DOUBLES
