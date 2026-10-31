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


def main():
    TEX.mkdir(parents=True, exist_ok=True)
    name_tag()
    print("assets generated in", TEX)


if __name__ == "__main__":
    main()
