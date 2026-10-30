"""Draw the pose silhouettes of the Onion module and write poses.h.

Each pose is a 1-bit mask of POSE_W x POSE_H, drawn from simple shapes (no
traced photographs), with the feet or the bottom of the frame at the bottom
edge. The module scales it to the photo area of the live view.

    python3 extras/onion/poses.py            # writes poses.h and poses.png
"""
from pathlib import Path

from PIL import Image, ImageDraw

W, H = 120, 180
HERE = Path(__file__).parent


S = 4  # draw at S times the size, then reduce: smooth edges


def canvas():
    im = Image.new("L", (W * S, H * S), 0)
    return im, ImageDraw.Draw(im)


def finish(im):
    """Reduce to W x H and keep pixels that are at least half covered."""
    return im.resize((W, H), Image.BOX).point(lambda v: 255 if v >= 128 else 0).convert("1")


def sc(points):
    return [(x * S, y * S) for x, y in points]


def poly(d, points):
    d.polygon(sc(points), fill=255)


def limb(d, *points, width):
    """A limb through the points, with round joints and ends."""
    pts = sc(points)
    d.line(pts, fill=255, width=round(width * S), joint="curve")
    r = width * S / 2
    for x, y in pts:
        d.ellipse([x - r, y - r, x + r, y + r], fill=255)


def head(d, cx, cy, rx, ry):
    d.ellipse([(cx - rx) * S, (cy - ry) * S, (cx + rx) * S, (cy + ry) * S], fill=255)


def body(d, cx=58, top=30, lean=0):
    """A full-body torso: shoulders, waist, hips. lean shifts the hips sideways."""
    poly(d, [(cx - 17, top), (cx + 17, top), (cx + 13 + lean, top + 38), (cx + 15 + lean, top + 60),
             (cx - 15 + lean, top + 60), (cx - 13 + lean, top + 38)])
    d.rounded_rectangle(sc([(cx - 18, top - 2), (cx + 18, top + 10)]), radius=5 * S, fill=255)


# ---- the poses -------------------------------------------------------------

def over_the_shoulder():
    """Close: the back to the camera, the head turned to look back over the
    right shoulder. The frame cuts the body."""
    im, d = canvas()
    poly(d, [(4, 180), (6, 132), (18, 108), (44, 98), (76, 96), (98, 102), (112, 118), (116, 180)])  # the back
    poly(d, [(60, 100), (82, 98), (84, 76), (66, 74)])                 # neck, turned
    head(d, 76, 54, 19, 25)
    head(d, 92, 64, 6, 8)                                              # the cheek and nose, turned back
    head(d, 88, 76, 6, 5)                                              # the chin
    head(d, 62, 44, 12, 14)                                            # hair at the back of the head
    return finish(im)


def looking_away():
    """Half body: square to the camera, arms relaxed, the head turned to look
    out of the frame on the left."""
    im, d = canvas()
    head(d, 58, 32, 13, 18)
    head(d, 46, 32, 5, 4)                                              # the nose: the face turns left
    head(d, 49, 42, 5, 4)                                              # the chin
    poly(d, [(52, 46), (66, 46), (67, 60), (51, 60)])
    poly(d, [(32, 180), (30, 112), (26, 76), (38, 62), (82, 62), (94, 76), (90, 112), (88, 180)])
    limb(d, (28, 72), (16, 120), (14, 168), width=13)                  # arms hang, apart from the body
    limb(d, (92, 72), (104, 120), (106, 168), width=13)
    return finish(im)


def walking_toward():
    """Full body, front view: mid-stride toward the camera. The near foot is
    lower, the far foot higher, and the arms swing against the legs."""
    im, d = canvas()
    head(d, 60, 13, 9, 11)
    poly(d, [(56, 20), (64, 20), (65, 30), (55, 30)])
    body(d, cx=60)
    limb(d, (44, 33), (36, 60), (42, 84), width=8)                     # arm swings forward: hand in front
    limb(d, (76, 33), (86, 58), (90, 76), width=8)                     # arm swings back
    limb(d, (52, 88), (50, 132), (48, 170), width=14)                  # near leg, foot low
    limb(d, (68, 88), (72, 126), (70, 156), width=12)                  # far leg, foot high
    limb(d, (40, 173), (56, 173), width=9)
    limb(d, (64, 158), (76, 158), width=8)
    return finish(im)


def figure(d, cx, top, scale):
    """A standing figure for a group: head at top, feet at the bottom."""
    k = scale
    head(d, cx, top + 9 * k, 7 * k, 9 * k)
    poly(d, [(cx - 13 * k, top + 22 * k), (cx + 13 * k, top + 22 * k), (cx + 12 * k, top + 78 * k),
             (cx - 12 * k, top + 78 * k)])
    d.rounded_rectangle(sc([(cx - 14 * k, top + 20 * k), (cx + 14 * k, top + 30 * k)]), radius=4 * S, fill=255)
    limb(d, (cx - 13 * k, top + 25 * k), (cx - 16 * k, top + 72 * k), width=7 * k)
    limb(d, (cx + 13 * k, top + 25 * k), (cx + 16 * k, top + 72 * k), width=7 * k)
    limb(d, (cx - 6 * k, top + 76 * k), (cx - 6 * k, 172), width=11 * k)
    limb(d, (cx + 6 * k, top + 76 * k), (cx + 6 * k, 172), width=11 * k)
    limb(d, (cx - 11 * k, 175), (cx - 3 * k, 175), width=6 * k)
    limb(d, (cx + 3 * k, 175), (cx + 11 * k, 175), width=6 * k)


def group_of_three():
    """Three people standing close, of staggered heights: tall in the middle,
    the others a little shorter."""
    im, d = canvas()
    figure(d, 60, 4, 1.0)
    figure(d, 23, 22, 0.86)
    figure(d, 97, 30, 0.8)
    return finish(im)


# The order of the POSE setting.
POSES = [
    ("over-the-shoulder", "Over the shoulder", over_the_shoulder),
    ("looking-away", "Looking away", looking_away),
    ("walking-toward", "Walking toward the camera", walking_toward),
    ("group-of-three", "Group of three", group_of_three),
]


def pack(im):
    """Rows of W bits, most significant bit first: W / 8 bytes a row."""
    px = im.load()
    out = []
    for y in range(H):
        for xb in range(0, W, 8):
            byte = 0
            for i in range(8):
                byte = byte << 1 | (1 if px[xb + i, y] else 0)
            out.append(byte)
    return out


def main():
    images = [(key, name, draw()) for key, name, draw in POSES]
    lines = [
        "/* Generated by extras/onion/poses.py. Do not edit: run python3 extras/onion/poses.py. */",
        "#ifndef ONION_POSES_H",
        "#define ONION_POSES_H",
        f"#define POSE_W {W}U",
        f"#define POSE_H {H}U",
        "#define POSE_ROW (POSE_W / 8U)",
        f"#define POSE_COUNT {len(images)}U",
        "/* One bit for each pixel, rows of POSE_ROW bytes, the left pixel in the high bit. */",
        "static const u8 poses[POSE_COUNT][POSE_ROW * POSE_H] = {",
    ]
    for key, name, im in images:
        data = pack(im)
        lines.append(f"    /* {name} */ {{")
        for i in range(0, len(data), 15):
            lines.append("        " + ", ".join(f"0x{b:02x}" for b in data[i:i + 15]) + ",")
        lines.append("    },")
    lines += ["};", "#endif", ""]
    (HERE / "poses.h").write_text("\n".join(lines))
    # A preview sheet, not used by the build.
    sheet = Image.new("L", (len(images) * (W + 10) + 10, H + 20), 40)
    for i, (_, _, im) in enumerate(images):
        sheet.paste(im.convert("L").point(lambda v: 230 if v else 70), (10 + i * (W + 10), 10))
    sheet.save(HERE / "poses.png")


if __name__ == "__main__":
    main()
