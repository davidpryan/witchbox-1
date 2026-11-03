// Playbox audio engine: the part that makes sound. Plain C interface, so the
// same code is built into the firmware (pbaudio.c runs it on the second
// core) and into a test program on a computer (firmware/tools/render.cpp).
// Nothing in here touches hardware or MicroPython.
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// 22,050 Hz, 16 bits, 32 clocks a frame: the same I2S timing as the test beep
// that is known to work with the PCM5102 on this build. It is also the rate
// of the recordings, so they play without resampling.
#define PB_SAMPLE_RATE 22050
#define PB_SEATS 4

enum {
    PB_NOTE = 1,        // seat, sound, note (MIDI number), vel 0..127, arg = hold time in ms (0 = the sound's own)
                        //   for drums: sound = DRUM_BASE + drum, note = semitones up or down, arg = kit
                        //   for "Found" and "Click": vel as usual, note = which one
    PB_SEAT_LEVEL,      // seat, vel = level 0..127
    PB_SEAT_TONE,       // seat, vel = brightness 0..127 (64 = as designed)
    PB_MASTER,          // vel = level 0..127
    PB_DROP,            // note = how far everything is pitched down, in sixteenths of a semitone
    PB_ECHO,            // vel = how much echo 0..127, arg = echo time in ms
    PB_ALL_OFF,         // let every held note go
};

typedef struct {
    uint8_t op, seat, sound, vel;
    int16_t note;
    uint16_t arg;
} pb_cmd_t;

void pb_init(void);
void pb_command(const pb_cmd_t *c);
void pb_render(int16_t *out, int frames);     // mono
int pb_active(void);                          // voices sounding now
int pb_shed(void);                            // drop the oldest note (the processor is overloaded)
const char *pb_sound_engine(int sound);       // which engine an instrument uses, for reports

#ifdef __cplusplus
}
#endif
