#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The emulator as one page with nothing to fetch but its worklet (for hosts that wrap the page in
their own document and serve files beside it): build/emu (web/emu/build.sh) ->
build/artifact/{witchbox1-emulator.html, worklet.js}. The page loses its document wrapper; witchbox1.wasm goes
into it as base64. (AMB-1's.)"""
import base64
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EMU, OUT = ROOT / "build/emu", ROOT / "build/artifact"
OUT.mkdir(parents=True, exist_ok=True)
s = (EMU / "index.html").read_text()
s = re.sub(r"<!doctype html>\s*<html[^>]*>\s*<head>\s*", "", s)
s = re.sub(r'<meta charset="utf-8">\s*<meta name="viewport"[^>]*>\s*', "", s)
s = s.replace("</head>\n<body>\n", "").replace("</body>\n</html>\n", "")
wasm = base64.b64encode((EMU / "witchbox1.wasm").read_bytes()).decode()
old = 'fetch("witchbox1.wasm").then((r) => { if (!r.ok) throw new Error(`witchbox1.wasm: ${r.status}`); return r.arrayBuffer(); })'
assert old in s
s = s.replace(old, "Promise.resolve(Uint8Array.from(atob(HAUNT_WASM), (c) => c.charCodeAt(0)).buffer)")
s = s.replace('<script>\n"use strict";', f'<script>\n"use strict";\nconst HAUNT_WASM = "{wasm}";', 1)
(OUT / "witchbox1-emulator.html").write_text(s)
(OUT / "worklet.js").write_text((EMU / "worklet.js").read_text())
print(f"artifact: {OUT / 'witchbox1-emulator.html'} ({len(s)} B), worklet.js")
