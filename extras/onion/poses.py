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
    d.line(pts, fill=255, width=width * S, joint="curve")
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


# ---- close poses: head and shoulders, the frame cuts the body ---------------

def head_shoulders():
    im, d = canvas()
    head(d, 60, 64, 21, 27)
    poly(d, [(51, 84), (69, 84), (71, 106), (49, 106)])
    poly(d, [(6, 180), (10, 132), (22, 112), (48, 102), (72, 102), (98, 112), (110, 132), (114, 180)])
    return finish(im)


def three_quarter():
    im, d = canvas()
    head(d, 62, 62, 19, 27)
    poly(d, [(76, 58), (86, 70), (78, 76)])                            # the nose: the head turns right
    poly(d, [(52, 84), (68, 84), (72, 106), (52, 106)])
    poly(d, [(18, 180), (20, 136), (30, 114), (52, 102), (74, 104), (90, 114), (96, 136), (94, 180)])  # one shoulder away
    return finish(im)


def hand_to_chin():
    im, d = canvas()
    head(d, 56, 60, 20, 26)
    poly(d, [(48, 80), (64, 80), (66, 102), (46, 102)])
    poly(d, [(4, 180), (8, 132), (20, 112), (44, 100), (66, 100), (86, 108), (96, 122), (98, 180)])
    limb(d, (112, 176), (104, 134), (78, 92), width=13)                 # elbow out, forearm up to the chin
    head(d, 72, 88, 10, 8)                                             # the hand
    return finish(im)


def arms_crossed():
    im, d = canvas()
    head(d, 60, 32, 14, 18)
    poly(d, [(54, 46), (66, 46), (67, 60), (53, 60)])
    poly(d, [(30, 180), (28, 110), (22, 74), (36, 60), (84, 60), (98, 74), (92, 110), (90, 180)])
    d.rounded_rectangle(sc([(10, 104), (110, 126)]), radius=11 * S, fill=255)   # the crossed arms, elbows out
    return finish(im)


# ---- full-body poses: the feet at the bottom ---------------------------------

def standing():
    im, d = canvas()
    head(d, 58, 13, 9, 11)
    poly(d, [(54, 20), (62, 20), (63, 30), (53, 30)])
    body(d, lean=-2)
    limb(d, (41, 33), (31, 62), (33, 90), width=8)                      # arm hangs, away from the body
    limb(d, (75, 33), (92, 56), (74, 74), width=8)                      # hand on the hip, elbow out
    limb(d, (49, 88), (48, 130), (47, 170), width=11)                   # the weight leg
    limb(d, (64, 88), (72, 128), (63, 168), width=10)                   # the relaxed leg
    limb(d, (40, 174), (54, 174), width=7)
    limb(d, (58, 170), (70, 170), width=7)
    return finish(im)


def leaning():
    im, d = canvas()
    poly(d, [(0, 0), (9, 0), (9, 180), (0, 180)])                      # the wall
    head(d, 30, 15, 9, 11)
    poly(d, [(27, 22), (35, 22), (37, 32), (27, 32)])
    poly(d, [(16, 30), (48, 32), (52, 66), (58, 90), (32, 92), (22, 64)])      # leans back on the wall
    limb(d, (18, 34), (12, 62), (14, 84), width=8)                      # arm against the wall
    limb(d, (46, 36), (64, 58), (52, 80), width=8)                      # hand in the pocket
    limb(d, (50, 88), (62, 130), (68, 170), width=11)                   # standing leg, out from the wall
    limb(d, (38, 88), (54, 128), (86, 166), width=10)                   # crossed leg
    limb(d, (62, 174), (78, 174), width=7)
    limb(d, (84, 168), (98, 172), width=7)
    return finish(im)


def seated():
    im, d = canvas()
    head(d, 46, 42, 9, 11)
    poly(d, [(42, 50), (50, 50), (51, 60), (41, 60)])
    poly(d, [(30, 58), (60, 58), (62, 90), (64, 114), (34, 114), (30, 90)])
    limb(d, (32, 62), (26, 92), (54, 106), width=8)                     # hands rest on the knees
    limb(d, (58, 62), (70, 90), (78, 106), width=8)
    limb(d, (40, 110), (90, 112), width=14)                             # thighs forward
    limb(d, (90, 112), (92, 166), width=11)                             # lower legs down
    limb(d, (80, 114), (74, 166), width=10)
    limb(d, (90, 172), (106, 172), width=7)
    limb(d, (70, 170), (84, 170), width=7)
    d.rectangle(sc([(22, 116), (72, 122)]), fill=255)                  # the stool
    limb(d, (28, 122), (22, 178), width=5)
    limb(d, (66, 122), (72, 178), width=5)
    return finish(im)


def walking():
    im, d = canvas()
    head(d, 60, 13, 9, 11)
    poly(d, [(56, 20), (64, 20), (65, 30), (55, 30)])
    body(d, cx=60)
    limb(d, (44, 34), (32, 58), (22, 76), width=8)                      # back arm swings back
    limb(d, (76, 34), (88, 58), (98, 74), width=8)                      # front arm swings forward
    limb(d, (52, 88), (36, 128), (20, 164), width=11)                   # back leg
    limb(d, (68, 88), (84, 128), (94, 168), width=11)                   # front leg
    limb(d, (10, 168), (22, 172), width=7)
    limb(d, (90, 174), (106, 174), width=7)
    return finish(im)


# The order of the POSE setting.
POSES = [
    ("head-shoulders", "Head and shoulders", head_shoulders),
    ("three-quarter", "Three-quarter turn", three_quarter),
    ("hand-chin", "Hand to chin", hand_to_chin),
    ("arms-crossed", "Arms crossed", arms_crossed),
    ("standing", "Standing", standing),
    ("leaning", "Leaning on a wall", leaning),
    ("seated", "Seated", seated),
    ("walking", "Walking", walking),
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
