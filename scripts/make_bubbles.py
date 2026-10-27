#!/usr/bin/env python3
"""Draws the monk's and the skeleton's speech bubbles in the zombies' style.

It takes the zombies' "I DON'T TEST!" bubble from the generated sheet, wipes
the lettering and sets new lettering in a bold condensed face, then writes
both on a white sheet to assets/source/bubbles.png. That sheet is committed,
so the cooked look does not depend on which fonts a machine has; run this
script again only to change the words.
"""

from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
import cook_sprites as cook  # noqa: E402

BUBBLE = (549, 789, 644, 853)  # the "I DON'T TEST!" bubble on the zombie sheet
FONTS = [
    "/System/Library/Fonts/Supplemental/DIN Alternate Bold.ttf",
    "/System/Library/Fonts/Supplemental/Arial Narrow Bold.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf",
]
WORDS = [("I WON'T", "PAIR!"), ("I LOVE", "PRS!")]
CELL = (130, 100)
SCALE = 4  # draw big, then shrink, for the soft edges of the zombies' lettering


def font(size):
    for path in FONTS:
        if Path(path).exists():
            return ImageFont.truetype(path, size)
    return ImageFont.load_default(size=size)


def blank_bubble():
    keyed = cook.key_background(Image.open(cook.SOURCE / cook.ZOMBIE).convert("RGB").crop(BUBBLE), "white")
    bubble = cook.tight(keyed)
    pixels = bubble.load()
    w, h = bubble.size
    # Inside the outline, anything dark is lettering: make it paper again.
    for y in range(5, h - 19):
        for x in range(5, w - 5):
            if pixels[x, y][3]:
                pixels[x, y] = (255, 255, 255, 255)
    return bubble


def lettered(bubble, lines):
    w, h = bubble.size
    big = Image.new("RGBA", (w * SCALE, h * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(big)
    face = font(17 * SCALE)
    pitch = 15 * SCALE
    top = (h - 14) * SCALE / 2 - pitch + 1 * SCALE
    for i, line in enumerate(lines):
        width = draw.textlength(line, font=face)
        draw.text(((w * SCALE - width) / 2, top + i * pitch + pitch / 2), line, font=face, fill=(20, 20, 20, 255), anchor="lm", stroke_width=SCALE // 2, stroke_fill=(20, 20, 20, 255))
    text = big.resize((w, h), Image.LANCZOS)
    out = bubble.copy()
    out.alpha_composite(text)
    return out


def main():
    sheet = Image.new("RGB", (CELL[0] * len(WORDS), CELL[1]), (255, 255, 255))
    base = blank_bubble()
    for i, lines in enumerate(WORDS):
        bubble = lettered(base, lines)
        sheet.paste(bubble, (i * CELL[0] + 18, 14), bubble)
    sheet.save(cook.SOURCE / "bubbles.png")
    print("wrote", cook.SOURCE / "bubbles.png")


if __name__ == "__main__":
    main()
