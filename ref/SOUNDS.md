# Sounds: what to build and where each one comes from

Status: built, not yet heard. The engine is in the firmware
(firmware/pbaudio) and every sound below has been rendered on a computer and
checked for level and tuning, but nobody has listened to one, on a computer
or on the Pico. Expect every sound to need adjusting by ear: each is one row
of numbers in firmware/pbaudio/engine.cpp.

What was built differs from the first plan in three ways:

- The mallets (Kalimba, Marimba, Toy Piano, Music Box) are FM, and the
  plucked strings are a small plucked-string model written for Playbox. The
  DaisySP string and modal models work out too much for every sample to
  share the Pico's second core with anything else.
- Only DaisySP's code is used (its drum models are ports of Plaits). Nothing
  was taken from the Mutable Instruments repository directly.
- Pitched instruments are all synthesized. The recordings are used for
  textures, found sounds and the voice hit; the piano, bell and choir
  recordings are in samples/ but not used, because their pitch is uncertain.

Sources and licences:

| Source | Licence | Used for |
|---|---|---|
| DaisySP (github.com/electro-smith/DaisySP) | MIT | filters, envelopes, oscillators, the voice (formant) oscillator, kick, snare and hi-hat models |
| Mutable Instruments Plaits, by way of DaisySP's ports | MIT | the drum models' design |
| pico-extras | BSD-3-Clause | the I2S output program |
| Recordings in samples/ (from Sonic Pi's bundled set) | CC0 | crackle, hum, whoosh, impacts, door, crow, found sounds, voice hit |

Not used: Sloop / Felucca engine code (GPL-3.0), Mutable's AVR modules
(GPL-3.0), DaisySP-LGPL.

Not done yet, and still only shown on screen: the toys' own tone knobs
(detune, crush, wobble, drive and so on), ducking of pads by the kick,
Gloom, swing, reverb. The engine has a per-seat brightness control and an
echo ready for these to use.

## Engines

| Engine | Built from | Voices at once |
|---|---|---|
| Analog | DaisySP band-limited oscillators (up to four), filter and envelope; glide, vibrato, drive, crush | 8 |
| FM | DaisySP two-operator FM with a falling modulation amount | 4 |
| String | Playbox's own plucked string (noise round a filtered delay) | 4 |
| Organ | five sine drawbars | 4 |
| Voice | two DaisySP formant oscillators and breath noise | 3 |
| Boom | DaisySP's 808-style bass drum, played at pitch | 2 |
| Recording | plays one of the CC0 recordings, looped or once, at any pitch | 6 |
| Noise | filtered noise, steady or swept | 3 |
| Drums | DaisySP kick (two kinds), snare, rim, three hi-hat/cymbal instances, toms and conga; Playbox's own clap, cowbell, shaker and click | one of each |

One echo is shared by everything. If more is asked for than the processor
can do, the oldest note is dropped.

## Instruments (40)

| Sound | Engine | Notes |
|---|---|---|
| Sub | Analog | sine, no filter movement |
| 808 Boom | Boom | long decay, played at pitch |
| 808 Slide | Boom | as Boom, with glide between notes |
| Acid | Analog | one saw, high resonance, fast filter envelope, glide |
| Reese | Analog | two saws detuned, low cutoff |
| Saw Bass | Analog | saw, short envelope |
| FM Bass | FM | ratio 1, short decay |
| Pluck Bass | String | low, damped |
| Soft Keys | FM | ratio 1, gentle |
| Electric Piano | FM | ratio 1 with a bell-like second partial; tremolo |
| Dull Piano | String | dark, with slow pitch wobble |
| Toy Piano | FM | bright, short |
| Clav | Analog | narrow pulse, fast decay |
| Music Box | FM | high, glassy |
| Soul Organ | Organ | lower drawbars |
| Church Organ | Organ | all drawbars, slow attack |
| Choir Aah | Voice | vowel "a", slow attack |
| Choir Ooh | Voice | vowel "o" |
| Warm Pad | Analog | two saws, low cutoff, slow envelope |
| Dark Strings | Analog, four saws | low cutoff |
| Glass Pad | FM | high ratio, slow attack |
| Saw Pad | Analog, four saws | open |
| Coven Saws | Analog, four saws | wide detune, filter opens over the note |
| Drone | Analog | one note, slow filter drift |
| Saw Lead | Analog | two saws |
| Square Lead | Analog | pulse |
| Chip Lead | Analog | square, crushed |
| Glide Lead | Analog | sine/triangle, long glide, vibrato |
| Screamer | Analog + `Overdrive` | sync sweep, pitch bend at the end |
| Whistle | Analog | sine with breath noise |
| Glass Bell | FM | non-whole ratio, long decay |
| Kalimba | FM | |
| Marimba | FM | |
| Harp | String | |
| Guitar | String | used by Strum |
| Trap Bell | FM | short, bright |
| Rave Stab | Analog, three saws | short |
| Minor Stab | Analog | |
| Ghost Voice | Voice | slides between notes |
| Screwed Voice | Voice | low, slides up into each note |

## Drums (20)

| Sound | From | Recording that can stand in |
|---|---|---|
| Kick | `synthetic_bass_drum` | |
| Long 808 Kick | `analog_bass_drum`, long decay | `kick_808.wav`, `kick_boom.wav` |
| Hard Kick | `synthetic_bass_drum` with drive | |
| Snare | `analog_snare_drum` | `snare.wav` |
| Rim | `synthetic_snare_drum`, short, no noise | `snap.wav` |
| Clap | new: three fast bursts of filtered noise, then a tail | |
| Long Clap | as Clap with a long tail | `clap_long.wav` |
| Closed Hat | `hi_hat`, short | `hat_closed.wav` |
| Open Hat | `hi_hat`, long; closed hat cuts it off | `hat_open.wav` |
| Pedal Hat | `hi_hat`, very short, darker | |
| Crash | `hi_hat` metallic source, long, bright | `crash.wav` |
| Ride | `hi_hat` metallic source, medium, with a ping | |
| Low, Mid, High Tom | `analog_bass_drum` at three pitches, less sweep | `tom_lo.wav`, `tom_mid.wav`, `tom_hi.wav` |
| Shaker | Noise, short band-passed burst | |
| Cowbell | new: two square waves through a band-pass | |
| Conga | Peaks `fm_drum` | |
| Click | Noise, one very short burst | `tick.wav`, `pop.wav` |
| Found Sound | Sample | `coins.wav`, `ting.wav`, `wood.wav`, `paper.wav`, `ping.wav`, `glass.wav` |

Kits (8) are the same sounds with different settings: 808, 909,
Acoustic-ish, Lo-fi, Witch, Garage, Chip, Tin.

## Textures

| Sound | From |
|---|---|
| Vinyl Crackle | Sample (`crackle.wav`, looped) plus `Dust` for extra pops; `skip.wav` for skips |
| Rain | Noise, high-passed hiss |
| Rumble | Noise, low-passed |
| City Hum | Sample (`hum.wav`, looped) |
| Noise Riser | Noise with the filter sweeping up; or Sample (`woosh.wav`) |
| Impact | Sample (`impact.wav`, `thud.wav`) |

## The Witchbox toys, one by one

As set in sound.py (the `WITCH` table), where they are easy to change.

| Toy | Plays |
|---|---|
| 41 Two-Step | Kick, Rim, Closed Hat, pop (recording) (Garage kit) |
| 42 Hat Roll | Closed Hat, Open Hat (Witch kit) |
| 43 Slow Stomp | Long Kick, Long Clap (Witch kit) |
| 44 Pocket Change | coins / ting (recording), wood (recording), paper (recording), ping / glass (recording) (Lo-fi kit) |
| 45 Limp | Kick, Snare (Lo-fi kit) |
| 46 Undertow | Sub, 1 octave down |
| 47 808 Slide | 808 Slide, 1 octave down |
| 48 Octave Pump | Saw Bass |
| 49 Reese | Reese |
| 50 Drone | Drone, 1 octave up |
| 51 Pump Pad | Warm Pad |
| 52 Coven Saws | Coven Saws |
| 53 Stutter Gate | Saw Pad |
| 54 Wet Piano | Dull Piano |
| 55 Cathedral | Church Organ |
| 56 Glass Arp | Glass Bell |
| 57 Bitcrush Arp | Chip Lead |
| 58 Hook Machine | Saw Lead |
| 59 Lost Melody | Whistle |
| 60 Screamer | Screamer |
| 61 Four Floor | Hard Kick, Clap, Closed Hat, Open Hat (909 kit) |
| 62 Snare Rush | Snare, Crash (909 kit) |
| 63 Broken Clock | Kick, snap (recording), tick (recording) (Lo-fi kit) |
| 64 Fog Toms | Low Tom, Mid Tom, High Tom (Witch kit) |
| 65 Riser | Riser, Glide Lead, Impact, Woosh (Witch kit) |
| 66 Rave Stab | Rave Stab |
| 67 Swell | Warm Pad |
| 68 Tape Loop | Soft Keys |
| 69 Crackle | pop (recording), Rain + Crackle, Rumble, Skip (Lo-fi kit) |
| 70 Night Bus | City Hum, Glide Lead, Woosh, Shaker, Door (Lo-fi kit) |
| 71 Music Box | Music Box, 1 octave down |
| 72 Siren | Glide Lead |
| 73 Shadow | Soft Keys |
| 74 Ghost Voice | Ghost Voice |
| 75 Stutter Chop | Vox Hit |
| 76 Screwed Voice | Screwed Voice |
| 77 Breath Stack | Choir Ooh |
| 78 Blackout | Riser (Witch kit) |
| 79 Screw | none of its own (it acts on the band) |
| 80 Arc | none of its own (it acts on the band) |

## Still to find or make

- Rain and passing traffic are made from noise; real recordings would sound
  better and freesound.org has CC0 ones.
- A reverb with a permissive licence.
- The recordings take about 1.3 MB of the firmware. It fits (the firmware is
  2.0 MB of the 2.5 MB set aside), but leaves little room for more
  recordings on a 4 MB Pico 2.
