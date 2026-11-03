#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""witchbox1's look on the site fm1-base makes (base/tools/make_site.py): the front page is app/site/index.html as it
is (the version and the package's name filled in), and the install and backup pages keep fm1-base's markup and
logic under witchbox1's colours, type and a site header.

    tools/site_skin.py SITE_DIR VERSION PKG
"""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
site, version, pkg = Path(sys.argv[1]), sys.argv[2], sys.argv[3]

FONT = '<link rel="preconnect" href="https://fonts.googleapis.com">\n<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow+Semi+Condensed:wght@400;500;600;700&display=swap">\n'
HEADER = '''<div class="wbtop"><a class="wbname" href="../">witchbox1</a><nav><a href="../install/">Install</a><a href="../emu/">Try it in the browser</a><a href="../manual/">Manual</a><a href="../backup/">Backup</a></nav></div>
'''
SKIN = '''<style>
/* witchbox1's skin (tools/site_skin.py): the page's own variables and rules, overridden */
:root, :root:not([data-theme="light"]), :root[data-theme="dark"], :root[data-theme="light"] {
  --bg: #14141a; --fg: #f8f2e2; --dim: #9a96a4; --accent: #ff54a4; --on-accent: #1a0a12; --bad: #ff7a6e; --focus: #3cc8dc;
  --ground: #14141a; --ink: #f8f2e2; --muted: #9a96a4; --rule: #424250; --teal: #3cc8dc; --card: #282832; --accent-ink: #ff8ac4;
  color-scheme: dark; }
html { background: #14141a; }
body { font-family: "Barlow Semi Condensed", "Barlow", "Arial Narrow", sans-serif !important; font-size: 18px !important; font-weight: 500; }
main, footer { max-width: 620px !important; }
h1 { font-size: 52px !important; font-weight: 700 !important; line-height: 1 !important; letter-spacing: -.02em; margin: 0 0 20px !important; }
h2 { font-size: 26px !important; font-weight: 700 !important; }
button, button.primary { background: var(--accent); color: var(--on-accent); font-family: inherit; font-weight: 700; font-size: 18px; border-radius: 4px !important; }
button.plain, button.quiet, label.pick { background: transparent; color: var(--fg); border: 1px solid var(--rule); }
.lang { color: var(--dim); }
.backup { border-color: var(--accent); }
.card, .status { background: var(--card); }
progress { accent-color: var(--accent); }
a { color: var(--fg); text-decoration-color: var(--dim); text-underline-offset: 3px; }
.wbtop { max-width: 620px; margin: 0 auto; padding: 22px 16px 0; display: flex; align-items: baseline; flex-wrap: wrap; gap: 20px; font: 500 17px "Barlow Semi Condensed", sans-serif; }
.wbtop .wbname { font-weight: 700; font-size: 22px; text-decoration: none; margin-right: auto; }
.wbtop nav a { color: var(--dim); text-decoration: none; margin-left: 18px; }
.wbtop nav a:hover { color: var(--fg); }
</style>
</head>'''

def skin(page: Path):
    s = page.read_text(encoding="utf-8")
    if "wbtop" in s:
        return
    s = s.replace("</head>", FONT + SKIN, 1)
    s = s.replace("<body>", "<body>\n" + HEADER, 1)
    page.write_text(s, encoding="utf-8")

front = (ROOT / "app" / "site" / "index.html").read_text(encoding="utf-8")
(site / "index.html").write_text(front.replace("{{VERSION}}", version).replace("{{PKG}}", pkg), encoding="utf-8")
for p in ("install/index.html", "backup/index.html"):
    if (site / p).exists():
        skin(site / p)
print(f"site skin: {site}/index.html, install/, backup/")
