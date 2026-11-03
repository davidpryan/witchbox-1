#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Make the web installer for a built package: a folder with one page and the package beside it.

  tools/make_installer.py build/witchbox1-0.1.fwsc 0.1 build/install
  python3 -m http.server -d build/install 8000        then open http://localhost:8000 in Chrome or Edge

The page is web/witchbox1_installer.html with fm1pkg.js and fm1ota.js (Felucca's update protocol, in the
browser over Web MIDI) and the package's identity inlined. It installs HAUNT-1 and can put M-VAVE's own
firmware back. Web MIDI needs a secure page: localhost counts, a file:// page may not."""
import json
import re
import shutil
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1]
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def main(pkg, version, out):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_\d+", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not an FM-1 package")
    if b"FELUCCA-LOADER-1" not in raw:
        raise SystemExit(f"{pkg}: no update loader in it")
    page = (SRC / "web" / "witchbox1_installer.html").read_text(encoding="utf-8")
    lib = strip_module((SRC / "web" / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((SRC / "web" / "fm1ota.js").read_text(encoding="utf-8"))
    meta = json.dumps({"version": version, "product": product, "pkg": pkg.name})
    for mark in ("/*LIB*/", "/*META*/"):
        if page.count(mark) != 1:
            raise SystemExit(f"witchbox1_installer.html must contain {mark} once")
    out.mkdir(parents=True, exist_ok=True)
    (out / "index.html").write_text(page.replace("/*LIB*/", lib).replace("/*META*/", meta), encoding="utf-8")
    if (out / pkg.name).resolve() != pkg.resolve():
        shutil.copy(pkg, out / pkg.name)
    print(f"installer: {out}/index.html and {pkg.name} ({len(raw)} B, {product})")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    main(*sys.argv[1:4])
