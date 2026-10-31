#!/usr/bin/env python3
"""Generates the procedural textures in assets/ (requires Pillow).

All art is drawn here so the repository has no third-party image licenses.
Run from the repository root:  python3 tools/generate_assets.py
"""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
TEX = ROOT / "assets" / "textures"
FONTS = ROOT / "assets" / "fonts"


def font(size, bold=False):
    return ImageFont.truetype(str(FONTS / ("DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf")), size)


def name_tag():
    w, h = 560, 120
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((2, 2, w - 3, h - 3), radius=18, fill=(12, 18, 38, 235), outline=(140, 200, 255, 255), width=3)
    d.text((24, 16), "Aleksa Ćetković", font=font(40, bold=True), fill=(255, 255, 255, 255))
    d.text((24, 68), "SV 77/2022", font=font(32), fill=(170, 215, 255, 255))
    img.save(TEX / "name_tag.png")


# --- Astronaut glove cursor -------------------------------------------------
# Drawn at 4x and downsampled for anti-aliasing. Both images share the hotspot
# at the tip of the index finger, see CURSOR_HOTSPOT in engine/src/Cursor.cpp.
GLOVE_SIZE = 64
SS = 4
OUTLINE = (40, 46, 60, 255)
SUIT = (236, 238, 242, 255)
SHADE = (196, 202, 214, 255)
CUFF = (240, 130, 40, 255)
CUFF_RING = (120, 130, 150, 255)


def _finger(d, x, y, w, h):
    s = SS
    d.rounded_rectangle((x * s, y * s, (x + w) * s, (y + h) * s), radius=w * s // 2,
                        fill=SUIT, outline=OUTLINE, width=2 * s)


def _glove(pointing):
    s = SS
    img = Image.new("RGBA", (GLOVE_SIZE * s, GLOVE_SIZE * s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # cuff (bottom)
    d.rounded_rectangle((16 * s, 50 * s, 46 * s, 63 * s), radius=3 * s, fill=CUFF, outline=OUTLINE, width=2 * s)
    d.rectangle((17 * s, 53 * s, 45 * s, 56 * s), fill=CUFF_RING)
    # thumb (open hand only; the pointing hand folds it over the palm below)
    if not pointing:
        d.rounded_rectangle((40 * s, 28 * s, 52 * s, 46 * s), radius=6 * s, fill=SUIT, outline=OUTLINE, width=2 * s)
    # fingers: index starts at x=14 so its tip is the hotspot (19, 1)
    if pointing:
        _finger(d, 14, 1, 10, 34)   # index extended
        for x in (23, 30, 37):       # middle, ring, little curled into knuckles
            d.ellipse((x * s, 19 * s, (x + 9) * s, 30 * s), fill=SUIT, outline=OUTLINE, width=2 * s)
    else:
        _finger(d, 14, 1, 10, 30)
        _finger(d, 23, 3, 9, 28)
        _finger(d, 31, 7, 8, 24)
        _finger(d, 38, 14, 7, 18)
    # palm
    d.rounded_rectangle((14 * s, 26 * s, 45 * s, 52 * s), radius=8 * s, fill=SUIT, outline=OUTLINE, width=2 * s)
    # padding detail on the palm (fabric seams)
    d.line((18 * s, 42 * s, 41 * s, 42 * s), fill=SHADE, width=2 * s)
    d.line((18 * s, 46 * s, 41 * s, 46 * s), fill=SHADE, width=2 * s)
    if pointing:  # thumb folded across the palm
        d.rounded_rectangle((20 * s, 30 * s, 40 * s, 38 * s), radius=4 * s, fill=SUIT, outline=OUTLINE, width=2 * s)
    return img.resize((GLOVE_SIZE, GLOVE_SIZE), Image.LANCZOS)


def cursors():
    out = ROOT / "assets" / "cursor"
    out.mkdir(parents=True, exist_ok=True)
    _glove(False).save(out / "glove_open.png")
    _glove(True).save(out / "glove_point.png")


# --- Scene textures (2D) ----------------------------------------------------
import math
import random

from PIL import ImageFilter


def _ss(w, h, s=2):
    """Supersampled RGBA canvas; caller resizes back by `s`."""
    return Image.new("RGBA", (w * s, h * s), (0, 0, 0, 0))


def space():
    """View through the station window: stars, nebula and a ringed planet."""
    w, h = 1024, 512
    rng = random.Random(77)
    img = Image.new("RGBA", (w, h))
    px = img.load()
    for y in range(h):
        t = y / h
        for x in range(w):
            px[x, y] = (int(4 + 10 * t), int(6 + 8 * t), int(18 + 26 * t), 255)
    neb = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    nd = ImageDraw.Draw(neb)
    for _ in range(26):
        cx, cy, r = rng.randint(0, w), rng.randint(0, h), rng.randint(40, 130)
        col = rng.choice([(120, 60, 170, 38), (40, 90, 180, 38), (180, 70, 120, 28)])
        nd.ellipse((cx - r, cy - r * 0.6, cx + r, cy + r * 0.6), fill=col)
    neb = neb.filter(ImageFilter.GaussianBlur(28))
    img = Image.alpha_composite(img, neb)
    d = ImageDraw.Draw(img)
    for _ in range(520):
        x, y = rng.randint(0, w - 1), rng.randint(0, h - 1)
        b = rng.randint(120, 255)
        r = 1 if rng.random() < 0.9 else 2
        d.ellipse((x - r / 2, y - r / 2, x + r / 2, y + r / 2), fill=(b, b, min(255, b + 20), 255))
    # ringed planet
    pl = _ss(w, h)
    pd = ImageDraw.Draw(pl)
    cx, cy, r = 740 * 2, 300 * 2, 110 * 2
    pd.ellipse((cx - r * 2.0, cy - r * 0.45, cx + r * 2.0, cy + r * 0.45), outline=(220, 190, 150, 200), width=14)
    for i in range(r, 0, -2):
        t = i / r
        col = (int(230 - 120 * t), int(150 - 60 * t), int(90 - 30 * t), 255)
        pd.ellipse((cx - i - (r - i) * 0.35, cy - i - (r - i) * 0.35, cx + i - (r - i) * 0.35, cy + i - (r - i) * 0.35), fill=col)
    # soft cloud bands clipped to the planet disc
    bands = _ss(w, h)
    bd = ImageDraw.Draw(bands)
    for k, a in [(-120, 50), (-50, 40), (30, 55), (100, 35)]:
        bd.rectangle((cx - r, cy + k - 14, cx + r, cy + k + 14), fill=(150, 80, 50, a))
    mask = Image.new("L", bands.size, 0)
    ImageDraw.Draw(mask).ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    bands.putalpha(Image.composite(bands.getchannel("A"), Image.new("L", bands.size, 0), mask))
    pl = Image.alpha_composite(pl, bands.filter(ImageFilter.GaussianBlur(6)))
    pd = ImageDraw.Draw(pl)
    # front half of the ring over the planet
    pd.arc((cx - r * 2.0, cy - r * 0.45, cx + r * 2.0, cy + r * 0.45), 0, 180, fill=(235, 205, 165, 230), width=14)
    pl = pl.resize((w, h), Image.LANCZOS)
    img = Image.alpha_composite(img, pl)
    # small moon
    d = ImageDraw.Draw(img)
    d.ellipse((180, 110, 230, 160), fill=(170, 175, 190, 255))
    d.ellipse((192, 122, 204, 134), fill=(140, 145, 160, 255))
    img.convert("RGB").save(TEX / "space.png")


def metal():
    """Tileable brushed metal panel with rivets (cabinet frame)."""
    n = 256
    rng = random.Random(3)
    img = Image.new("RGB", (n, n))
    px = img.load()
    for y in range(n):
        row = rng.randint(-6, 6)
        for x in range(n):
            v = 150 + row + rng.randint(-4, 4)
            px[x, y] = (v - 8, v - 2, v + 8)
    d = ImageDraw.Draw(img)
    d.rectangle((0, 0, n - 1, n - 1), outline=(95, 100, 115), width=3)
    for (x, y) in [(14, 14), (n - 15, 14), (14, n - 15), (n - 15, n - 15)]:
        d.ellipse((x - 6, y - 6, x + 6, y + 6), fill=(110, 116, 130), outline=(80, 84, 96))
        d.ellipse((x - 3, y - 4, x + 1, y), fill=(200, 205, 215))
    img.save(TEX / "metal.png")


def wall():
    """Tileable back wall: dark panels with seams and a small vent."""
    n = 256
    rng = random.Random(5)
    img = Image.new("RGB", (n, n))
    px = img.load()
    for y in range(n):
        for x in range(n):
            v = rng.randint(-3, 3)
            px[x, y] = (34 + v, 44 + v, 58 + v)
    d = ImageDraw.Draw(img)
    d.rectangle((0, 0, n - 1, n - 1), outline=(22, 28, 38), width=4)
    d.line((0, n // 2, n, n // 2), fill=(26, 33, 44), width=3)
    for x in range(40, 120, 12):
        d.line((x, 150, x, 200), fill=(24, 30, 40), width=4)
    img.save(TEX / "wall.png")


def pot():
    """Front view of a white space-station pot (soil is drawn separately)."""
    w, h, s = 256, 192, 3
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    # rim
    d.rounded_rectangle((4 * s, 4 * s, (w - 4) * s, 40 * s), radius=10 * s, fill=(232, 236, 242, 255), outline=(60, 66, 80, 255), width=4 * s)
    # body (trapezoid)
    d.polygon([(22 * s, 38 * s), ((w - 22) * s, 38 * s), ((w - 44) * s, (h - 4) * s), (44 * s, (h - 4) * s)],
              fill=(214, 220, 230, 255), outline=(60, 66, 80, 255))
    d.line([(22 * s, 38 * s), (44 * s, (h - 4) * s), ((w - 44) * s, (h - 4) * s), ((w - 22) * s, 38 * s)], fill=(60, 66, 80, 255), width=4 * s)
    # stripe and status light
    d.polygon([(30 * s, 90 * s), ((w - 30) * s, 90 * s), ((w - 34) * s, 108 * s), (34 * s, 108 * s)], fill=(70, 140, 220, 255))
    d.ellipse(((w // 2 - 10) * s, 130 * s, (w // 2 + 10) * s, 150 * s), fill=(90, 200, 120, 255), outline=(40, 70, 50, 255), width=2 * s)
    # shading on the right side
    sh = _ss(w, h, s)
    sd = ImageDraw.Draw(sh)
    sd.polygon([((w - 80) * s, 40 * s), ((w - 24) * s, 40 * s), ((w - 46) * s, (h - 6) * s), ((w - 90) * s, (h - 6) * s)], fill=(0, 0, 30, 40))
    img = Image.alpha_composite(img, sh)
    img.resize((w, h), Image.LANCZOS).save(TEX / "pot.png")


def soil():
    """Soil strip seen above the pot rim; tinted by moisture in the shader."""
    w, h = 256, 48
    rng = random.Random(9)
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((0, 0, w - 1, h - 1), fill=(200, 200, 200, 255))
    for _ in range(260):
        x, y = rng.randint(10, w - 10), rng.randint(6, h - 6)
        if ((x - w / 2) / (w / 2)) ** 2 + ((y - h / 2) / (h / 2)) ** 2 < 0.85:
            v = rng.randint(140, 255)
            d.ellipse((x - 2, y - 2, x + 2, y + 2), fill=(v, v, v, 255))
    img.save(TEX / "soil.png")


def _leaf(d, cx, cy, length, width, angle, fill, outline, s):
    pts = []
    for i in range(0, 41):
        t = i / 40
        x = t * length
        y = math.sin(math.pi * t) * width * (1 if i <= 20 else 1)
        pts.append((x, y))
    pts += [(x, -y) for (x, y) in reversed(pts)]
    a = math.radians(angle)
    poly = [((cx + x * math.cos(a) - y * math.sin(a)) * s, (cy - (x * math.sin(a) + y * math.cos(a))) * s) for x, y in pts]
    d.polygon(poly, fill=fill, outline=outline)
    d.line([(cx * s, cy * s), ((cx + length * 0.9 * math.cos(a)) * s, (cy - length * 0.9 * math.sin(a)) * s)], fill=outline, width=2 * s)


def plants():
    w, h, s = 256, 320, 3
    stem = (60, 130, 60, 255)
    leaf = (90, 190, 90, 255)
    leaf2 = (70, 165, 80, 255)
    edge = (30, 80, 40, 255)
    base_y = h - 10

    # seed: a seed resting in a small soil mound
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    d.ellipse((88 * s, (base_y - 26) * s, 168 * s, (base_y + 14) * s), fill=(110, 75, 45, 255), outline=(60, 40, 25, 255), width=2 * s)
    d.ellipse((114 * s, (base_y - 40) * s, 142 * s, (base_y - 16) * s), fill=(200, 160, 90, 255), outline=(90, 60, 30, 255), width=2 * s)
    d.line((122 * s, (base_y - 34) * s, 134 * s, (base_y - 24) * s), fill=(150, 110, 60, 255), width=2 * s)
    img.resize((w, h), Image.LANCZOS).save(TEX / "plant_seed.png")

    # sprout: short stem with two leaves
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    d.line((128 * s, base_y * s, 128 * s, (base_y - 110) * s), fill=stem, width=7 * s)
    _leaf(d, 128, base_y - 100, 70, 22, 150, leaf, edge, s)
    _leaf(d, 128, base_y - 108, 76, 24, 30, leaf2, edge, s)
    _leaf(d, 128, base_y - 60, 40, 12, 160, leaf2, edge, s)
    img.resize((w, h), Image.LANCZOS).save(TEX / "plant_sprout.png")

    # mature: bushy plant with glowing star fruit
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    d.line((128 * s, base_y * s, 128 * s, (base_y - 240) * s), fill=stem, width=9 * s)
    d.line((128 * s, (base_y - 120) * s, 70 * s, (base_y - 200) * s), fill=stem, width=6 * s)
    d.line((128 * s, (base_y - 140) * s, 188 * s, (base_y - 220) * s), fill=stem, width=6 * s)
    for (x, y, l, wd, a, c) in [
        (128, base_y - 40, 84, 26, 160, leaf2), (128, base_y - 50, 84, 26, 20, leaf),
        (128, base_y - 100, 80, 24, 150, leaf), (128, base_y - 110, 80, 24, 35, leaf2),
        (70, base_y - 200, 56, 18, 130, leaf), (188, base_y - 220, 56, 18, 50, leaf2),
        (128, base_y - 236, 60, 20, 100, leaf), (128, base_y - 236, 52, 16, 60, leaf2),
        (128, base_y - 180, 64, 20, 165, leaf2), (128, base_y - 190, 64, 20, 15, leaf),
    ]:
        _leaf(d, x, y, l, wd, a, c, edge, s)
    glow = _ss(w, h, s)
    gd = ImageDraw.Draw(glow)
    fruits = [(64, base_y - 214), (196, base_y - 236), (100, base_y - 150), (160, base_y - 170), (128, base_y - 268)]
    for (x, y) in fruits:
        gd.ellipse(((x - 22) * s, (y - 22) * s, (x + 22) * s, (y + 22) * s), fill=(255, 210, 90, 90))
    glow = glow.filter(ImageFilter.GaussianBlur(8 * s))
    img = Image.alpha_composite(img, glow)
    d = ImageDraw.Draw(img)
    for (x, y) in fruits:
        d.ellipse(((x - 12) * s, (y - 12) * s, (x + 12) * s, (y + 12) * s), fill=(255, 190, 60, 255), outline=(170, 90, 20, 255), width=2 * s)
        d.ellipse(((x - 6) * s, (y - 8) * s, (x - 1) * s, (y - 3) * s), fill=(255, 245, 200, 255))
    img.resize((w, h), Image.LANCZOS).save(TEX / "plant_mature.png")


def lamp():
    w, h, s = 512, 64, 2
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((2 * s, 2 * s, (w - 2) * s, 44 * s), radius=12 * s, fill=(90, 98, 115, 255), outline=(40, 44, 56, 255), width=3 * s)
    d.rectangle((24 * s, 40 * s, (w - 24) * s, (h - 4) * s), fill=(255, 255, 255, 255), outline=(40, 44, 56, 255), width=2 * s)
    for x in range(40, w - 40, 32):
        d.line((x * s, 12 * s, (x + 16) * s, 12 * s), fill=(60, 66, 80, 255), width=3 * s)
    img.resize((w, h), Image.LANCZOS).save(TEX / "lamp.png")


def beam():
    """White trapezoid with alpha fading downwards; tinted and blended by the renderer."""
    w, h = 256, 256
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = img.load()
    for y in range(h):
        t = y / (h - 1)            # 0 at top (lamp), 1 at bottom
        half = (0.38 + 0.12 * t) * w
        a = int(255 * (1.0 - 0.65 * t))
        for x in range(w):
            dx = abs(x - w / 2)
            if dx <= half:
                edge = min(1.0, (half - dx) / 18.0)
                px[x, y] = (255, 255, 255, int(a * edge))
    img.save(TEX / "beam.png")


def tank():
    """Water tank frame; the glass window is transparent so the water quad behind shows through."""
    w, h, s = 256, 512, 2
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((4 * s, 4 * s, (w - 4) * s, (h - 4) * s), radius=36 * s, fill=(170, 178, 192, 255), outline=(50, 56, 70, 255), width=4 * s)
    # window cut-out (fully transparent)
    d.rounded_rectangle((44 * s, 70 * s, (w - 44) * s, (h - 60) * s), radius=20 * s, fill=(0, 0, 0, 0), outline=(50, 56, 70, 255), width=4 * s)
    # cap and label plate
    d.rounded_rectangle((70 * s, 16 * s, (w - 70) * s, 52 * s), radius=8 * s, fill=(70, 140, 220, 255), outline=(40, 60, 90, 255), width=2 * s)
    for y in range(120, h - 80, 60):  # level marks
        d.line(((w - 64) * s, y * s, (w - 44) * s, y * s), fill=(50, 56, 70, 255), width=3 * s)
    d.text((88 * s, 20 * s), "H2O", font=font(22 * s, bold=True), fill=(255, 255, 255, 255))
    img = img.resize((w, h), Image.LANCZOS)
    img.save(TEX / "tank.png")
    # glass highlight drawn over the water
    g = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    gd = ImageDraw.Draw(g)
    gd.rounded_rectangle((44, 70, w - 44, h - 60), radius=20, fill=(200, 230, 255, 30))
    gd.polygon([(60, 90), (90, 90), (90, h - 80), (60, h - 80)], fill=(255, 255, 255, 50))
    g.save(TEX / "tank_glass.png")


def droplet():
    w, h, s = 32, 48, 4
    img = _ss(w, h, s)
    d = ImageDraw.Draw(img)
    d.ellipse((4 * s, 16 * s, 28 * s, 44 * s), fill=(110, 180, 255, 255), outline=(40, 90, 170, 255), width=2 * s)
    d.polygon([(16 * s, 2 * s), (5 * s, 26 * s), (27 * s, 26 * s)], fill=(110, 180, 255, 255))
    d.line([(16 * s, 2 * s), (5 * s, 26 * s)], fill=(40, 90, 170, 255), width=2 * s)
    d.line([(16 * s, 2 * s), (27 * s, 26 * s)], fill=(40, 90, 170, 255), width=2 * s)
    d.ellipse((10 * s, 24 * s, 15 * s, 32 * s), fill=(230, 245, 255, 255))
    img.resize((w, h), Image.LANCZOS).save(TEX / "droplet.png")


def main():
    TEX.mkdir(parents=True, exist_ok=True)
    name_tag()
    cursors()
    space()
    metal()
    wall()
    pot()
    soil()
    plants()
    lamp()
    beam()
    tank()
    droplet()
    print("assets generated in", TEX)


if __name__ == "__main__":
    main()
