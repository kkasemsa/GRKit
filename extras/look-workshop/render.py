"""Develop a GR DNG with a GRKit effect preview table, as grkit.app does.

LibRaw: half size, camera white balance, linear sRGB, 16 bits, no auto
brightness, camera white level. Then each pixel, sRGB-encoded as the shaper,
is looked up in the effect's 33-point table (red fastest), trilinear.
"""
import json
import sys
from pathlib import Path

import numpy as np
import rawpy
from PIL import Image

import os
EFFECTS = Path(os.environ.get("GRKIT_PREVIEW_EFFECTS", "site-effects/gr3-2.10"))
MAX_EDGE = 1200


def catalog():
    c = json.loads((EFFECTS / "catalog.json").read_text())
    p = c["preview"]
    assert p["input"] == "linear-srgb16" and p["order"] == "rgb" and p["shaper"] == "srgb"
    return c


def load_table(rel, size):
    t = np.frombuffer((EFFECTS / rel).read_bytes(), dtype=np.uint8)
    assert t.size == size**3 * 3
    # index = (b * size + g) * size + r  (red fastest)
    return t.reshape(size, size, size, 3).astype(np.float32)  # [b, g, r, c]


def decode(dng):
    with rawpy.imread(str(dng)) as raw:
        rgb = raw.postprocess(
            half_size=True,
            use_camera_wb=True,
            output_color=rawpy.ColorSpace.sRGB,
            output_bps=16,
            gamma=(1, 1),
            no_auto_bright=True,
            adjust_maximum_thr=0.0,
        )
    return rgb  # H x W x 3, uint16, oriented


def resize_linear(rgb, max_edge=MAX_EDGE):
    h, w, _ = rgb.shape
    scale = min(1.0, max_edge / max(h, w))
    if scale == 1.0:
        return rgb
    nw, nh = max(1, round(w * scale)), max(1, round(h * scale))
    # Area resampling in linear light: Pillow's BOX filter on float channels.
    out = np.stack(
        [np.asarray(Image.fromarray(rgb[..., c].astype(np.float32), "F").resize((nw, nh), Image.BOX)) for c in range(3)],
        axis=-1,
    )
    return np.clip(np.rint(out), 0, 65535).astype(np.uint16)


def shaper(v):
    x = v.astype(np.float64) / 65535.0
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1 / 2.4) - 0.055)


def apply(rgb16, table, size):
    s = shaper(rgb16) * (size - 1)
    i = np.minimum(np.floor(s).astype(np.int64), size - 2)
    f = (s - i).astype(np.float32)
    r, g, b = i[..., 0], i[..., 1], i[..., 2]
    fr, fg, fb = f[..., 0:1], f[..., 1:2], f[..., 2:3]
    T = table

    def at(db, dg, dr):
        return T[b + db, g + dg, r + dr]

    c00 = at(0, 0, 0) + (at(0, 0, 1) - at(0, 0, 0)) * fr
    c10 = at(0, 1, 0) + (at(0, 1, 1) - at(0, 1, 0)) * fr
    c01 = at(1, 0, 0) + (at(1, 0, 1) - at(1, 0, 0)) * fr
    c11 = at(1, 1, 0) + (at(1, 1, 1) - at(1, 1, 0)) * fr
    c0 = c00 + (c10 - c00) * fg
    c1 = c01 + (c11 - c01) * fg
    out = c0 + (c1 - c0) * fb
    return np.clip(np.rint(out), 0, 255).astype(np.uint8)


def develop(dng, effect_id=None):
    c = catalog()
    size = c["preview"]["size"]
    rel = c["preview"]["baseline"] if effect_id is None else next(e["preview"] for e in c["effects"] if e["id"] == effect_id)
    rgb = resize_linear(decode(dng))
    return Image.fromarray(apply(rgb, load_table(rel, size), size), "RGB")


if __name__ == "__main__":
    develop(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] != "standard" else None).save(sys.argv[3])
