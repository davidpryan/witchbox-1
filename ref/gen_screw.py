#!/usr/bin/env python3
"""Regenerates firmware/src/app/wb/toys/screw_waver.h (see the note in it)."""
import os
from math import sin, pi
rows=[]
for w in range(11):
    trans=[]; last=None
    for d in range(6000):
        v=int(sin(d*pi/3000)*w*0.4)
        if v!=last: trans.append((d,v)); last=v
    rows.append(trans)
mx=max(len(r) for r in rows)
out=['/* Screw: Playbox\'s waver, int(sin(drift * pi / 3000) * waver * 0.4) for drift 0..5999 ms, as the',
     ' * points where the value changes (there is no double on the device). ref/gen_screw.py. */',
     '#define SR_TRANS %d'%mx,'static const int16_t SR_WAVER[11][SR_TRANS][2] = {']
for r in rows:
    r=r+[(6000,0)]*(mx-len(r))
    out.append('    {'+', '.join('{%d, %d}'%t for t in r)+'},')
out.append('};')
p=os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','firmware','src','app','wb','toys','screw_waver.h')
open(p,'w').write('\n'.join(out)+'\n')
