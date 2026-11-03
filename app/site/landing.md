# witchbox1

The eighty Witchbox toys of Playbox, as firmware for the M-VAVE FM-1: limping garage beats, sub bass, detuned saws,
glass arpeggios, screwed voices, record crackle, Turing machines, Markov chains and cellular automata for drums, and
conductors that cut the band, fog it, dub it out or move its key. Eight toys at a time on one clock, with a Tape or a
Clock to say who plays when.

![The band page: eight seats with their toys and seeds](manual/img/screen-band.png "witchbox1 on the FM-1's screen")

Version {{VERSION}}. Installing is at your own risk; the installer can put M-VAVE's own firmware back, and offers
m-bootloader, which keeps a recovery menu (PLAY held at power-on) even when a firmware will not start.

- [Install](install/): from Chrome or Edge, with the FM-1 connected by USB.
- [Manual](manual/): the panel, the keys as pads, the screen, the toys, saving, rescue.
- [Download {{PKG}}](firmware/{{PKG}}): for the command-line installer, `python3 base/tools/fm1_install.py {{PKG}}`.
- [Backup](backup/): the band and the saved toys to a file, and back.
- [Source](https://github.com/davidpryan/witchbox-1): GPL-3.0-only. Built on fm1-base and felucca-bsp, from Felucca by
  Leo Kuroshita, Hügelton Instruments; the frame is HAUNT-1's and AMB-1's.

M-VAVE and FM-1 are trademarks of their owners. witchbox1 is not affiliated with M-VAVE.
