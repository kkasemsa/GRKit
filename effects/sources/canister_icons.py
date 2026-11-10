#!/usr/bin/env python3
"""Draws the film canister icons of the effects and writes them into each
effect.json as an `image` icon (effects/README.md, Icons). The Recipe
pages (extras/film/*) use the same canisters as their tiles:
scripts/film-pages.py draws them from STOCKS.

Each canister is a generic 35 mm cassette: metal caps, a coloured body, a
coloured band with one letter, and the felt lip. The colours and the letter
are ours and only hint at the stock; no brand's logo or box design is drawn.
The icon is drawn at 10x and reduced to 40 x 40, so the shading survives.

    python3 effects/sources/canister_icons.py            # write the icons
    python3 effects/sources/canister_icons.py --sheet /tmp/sheet.png

Needs Pillow and numpy. The font is the repository's RobotoCondensed-Bold.
"""
import base64
import json
import sys
import zlib
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
FONT = ROOT / "crates/grkit-core/assets/RobotoCondensed-Bold.ttf"
EFFECTS = ROOT / "effects"
S = 10
N = 40 * S

# effect id: (body colour, band colour, letter)
STOCKS = {
    "portra-400-endura-v1": ((230, 165, 45), (120, 75, 45), "P"),
    "portra-800-push2-v1": ((215, 140, 40), (90, 55, 40), "P"),
    "ektar-100-edge-v1": ((225, 140, 35), (200, 35, 30), "E"),
    "gold-200-premier-v1": ((240, 195, 30), (235, 120, 25), "G"),
    "ultramax-400-ultra-v1": ((80, 120, 200), (245, 200, 40), "U"),
    "kodachrome-64-v1": ((225, 40, 40), (240, 220, 60), "K"),
    "ektachrome-e100-v1": ((40, 110, 190), (240, 220, 60), "E"),
    "trix-400-mono-v1": ((230, 200, 40), (30, 30, 30), "T"),
    "vision3-500t-supra-v1": ((60, 60, 65), (240, 170, 40), "V"),
    "vision3-250d-2383-v1": ((60, 60, 65), (90, 160, 230), "V"),
    "double-x-5222-mono-v1": ((60, 60, 65), (230, 230, 225), "X"),
    "c200-premier-v1": ((40, 140, 90), (235, 235, 230), "C"),
    "xtra-400-crystal-v1": ((40, 140, 90), (240, 200, 40), "S"),
    "pro-400h-crystal-v1": ((120, 180, 140), (235, 235, 230), "H"),
    "provia-100f-v1": ((40, 90, 170), (235, 235, 230), "P"),
    "velvia-100-v1": ((60, 150, 80), (240, 120, 30), "V"),
    "astia-100f-v1": ((150, 110, 170), (235, 235, 230), "A"),
    "eterna-500t-3523xd-v1": ((70, 70, 80), (180, 40, 40), "E"),
    "hp5-plus-400-mono-v1": ((235, 235, 235), (30, 30, 30), "H"),
    "red-filter-mono-v1": ((230, 200, 40), (200, 35, 30), "T"),
    "aerocolor-iv-v1": ((90, 150, 200), (235, 235, 230), "A"),
    "instax-v1": ((245, 245, 240), (90, 170, 230), "I"),
}


def shade_cyl(w, h, base):
    """A horizontal cylinder: brighter towards the left of centre."""
    x = np.linspace(-1, 1, w)
    profile = np.clip(1 - 0.55 * x**2 + 0.25 * np.exp(-((x + 0.35) / 0.18) ** 2), 0, 1.25)
    a = np.zeros((h, w, 3))
    for c in range(3):
        a[:, :, c] = np.clip(base[c] * profile[None, :], 0, 255)
    return Image.fromarray(a.astype(np.uint8), "RGB")


def canister(body, band, letter):
    im = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    draw = ImageDraw.Draw(im)
    # The menu frame of the native icons: a dark square with a grey edge.
    draw.rectangle([1 * S, 2 * S, 39 * S - 1, 38 * S - 1], fill=(51, 51, 51, 255), outline=(128, 128, 128, 255), width=S)
    left, right, top, bottom = 95, 305, 70, 350
    metal = (185, 185, 180)
    im.paste(shade_cyl(60, 30, (150, 150, 145)), (170, 40))
    im.paste(shade_cyl(right - left + 10, 28, metal), (left - 5, top))
    im.paste(shade_cyl(right - left + 10, 28, metal), (left - 5, bottom - 28))
    im.paste(shade_cyl(right - left, bottom - top - 56, body), (left, top + 28))
    im.paste(shade_cyl(right - left, 120, band), (left, top + 92))
    draw.rectangle([right - 4, top + 40, right + 18, bottom - 40], fill=(20, 20, 20, 255))
    draw.rectangle([right + 18, top + 70, right + 40, bottom - 70], fill=(70, 58, 40, 255))
    font = ImageFont.truetype(str(FONT), 150)
    box = draw.textbbox((0, 0), letter, font=font)
    tw, th = box[2] - box[0], box[3] - box[1]
    ink = (255, 255, 255, 255) if sum(band) < 400 else (20, 20, 20, 255)
    draw.text((left + (right - left - tw) / 2 - box[0], top + 92 + (120 - th) / 2 - box[1]), letter, font=font, fill=ink)
    small = im.resize((40, 40), Image.LANCZOS)
    # Alpha is 0 or 255: the frame has no soft edge in the native icons.
    a = np.asarray(small).copy()
    a[:, :, 3] = np.where(a[:, :, 3] >= 128, 255, 0)
    a[a[:, :, 3] == 0] = 0
    return Image.fromarray(a, "RGBA")


def encode(im):
    return base64.b64encode(zlib.compress(im.tobytes(), 9)).decode()


def main():
    sheet = "--sheet" in sys.argv
    icons = {}
    for effect_id, (body, band, letter) in STOCKS.items():
        icons[effect_id] = canister(body, band, letter)
        path = EFFECTS / effect_id / "effect.json"
        if not path.exists():
            print(f"skip {effect_id}: no effect.json", file=sys.stderr)
            continue
        if not sheet:
            text = path.read_text()
            definition = json.loads(text)
            definition["icon"] = {"template": "image", "rgba": encode(icons[effect_id])}
            path.write_text(json.dumps(definition, indent=2, ensure_ascii=False) + "\n")
    if sheet:
        out = sys.argv[sys.argv.index("--sheet") + 1]
        ids = list(icons)
        im = Image.new("RGB", (len(ids) * 170 + 10, 230), (60, 60, 60))
        draw = ImageDraw.Draw(im)
        font = ImageFont.truetype(str(FONT), 14)
        for i, effect_id in enumerate(ids):
            icon = icons[effect_id]
            im.paste(icon.resize((160, 160), Image.NEAREST), (10 + i * 170, 10), icon.resize((160, 160), Image.NEAREST))
            im.paste(icon, (70 + i * 170, 180), icon)
            draw.text((10 + i * 170, 212), effect_id[:22], font=font, fill=(220, 220, 220))
        im.save(out)


if __name__ == "__main__":
    main()
