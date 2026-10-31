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


def main():
    TEX.mkdir(parents=True, exist_ok=True)
    name_tag()
    cursors()
    print("assets generated in", TEX)


if __name__ == "__main__":
    main()
