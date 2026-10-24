"""Cook the AI-generated JPEG sprite sheets in assets/source into transparent
PNGs and a manifest the game loads (assets_cooked/sprites).

Each sheet is cut by hand-measured rectangles (see SPEC below). Inside a
"band" every blob of ink becomes one frame, left to right. The white (or sky)
background is keyed out by flood fill from the rectangle's border, so white
eyes and speech-bubble interiors survive. The manifest line is

    name file width height anchor_x anchor_y

where the anchor is the middle of the feet (the bottom row of the sprite).
"""

import argparse
from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "source"
OUT = ROOT / "assets_cooked" / "sprites"

BASE = "fabio_base.jpeg"
GUN = "fabio_pistol.jpeg"
SPELL = "fabio_spell_zombies.jpeg"
ZOMBIE = "fabio_zombie_attack.jpeg"
WORLD = "world_elements.jpeg"

# kind "band": every blob in the rectangle, left to right, named in order.
# kind "one": the rectangle is one sprite.
# bg "white" and "sky" are keyed out, "none" keeps the rectangle opaque.
SPEC = [
    # Fabio, base sheet
    ("band", BASE, (30, 36, 520, 160), ["fab_idle_%d" % i for i in range(5)], "white"),
    ("band", BASE, (590, 36, 1024, 160), ["fab_front_%d" % i for i in range(4)], "white"),
    ("band", BASE, (30, 184, 1024, 304), ["fab_walkA_%d" % i for i in range(10)], "white"),
    ("band", BASE, (30, 324, 1024, 444), ["fab_walkB_%d" % i for i in range(10)], "white"),
    ("band", BASE, (30, 440, 1024, 582), ["fab_jump_%d" % i for i in range(5)], "white"),
    ("band", BASE, (30, 588, 1024, 716), ["fab_attack_%d" % i for i in range(6)], "white"),
    ("band", BASE, (30, 722, 1024, 848), ["fab_hurt_%d" % i for i in range(4)], "white"),
    ("band", BASE, (30, 856, 1024, 984), ["fab_run_%d" % i for i in range(8)], "white"),
    # Fabio with the pistol, the potion and the spell
    ("band", GUN, (524, 466, 1024, 584), ["fab_gunwalk_%d" % i for i in range(5)], "white"),
    ("one", GUN, "fab_gunaim", (540, 608, 664, 715), "white"),
    ("one", GUN, "fab_gunshoot", (690, 608, 782, 715), "white"),
    ("one", GUN, "fx_muzzle", (782, 606, 864, 700), "white"),
    ("one", GUN, "fx_bullet", (866, 636, 924, 676), "white"),
    ("band", GUN, (830, 334, 1000, 412), ["pistol_0", "pistol_1"], "white"),
    ("band", GUN, (30, 863, 520, 984),
     ["fab_potion_0", "fab_potion_1", "fab_drink", "fab_aura"], "white"),
    ("band", GUN, (536, 866, 1024, 984), ["fab_cast_%d" % i for i in range(4)], "white"),
    ("one", SPELL, "fab_spell", (530, 465, 628, 582), "white"),
    ("one", SPELL, "fx_ray", (628, 494, 734, 545), "white"),
    # Bugs
    ("one", GUN, "bug_beetle", (812, 36, 910, 146), "white"),
    ("one", GUN, "bug_fly", (910, 36, 1020, 124), "white"),
    ("one", GUN, "bug_bee", (916, 134, 1010, 220), "white"),
    ("one", GUN, "bug_dark", (812, 172, 910, 284), "white"),
    ("one", GUN, "bug_small", (926, 226, 1000, 300), "white"),
    ("one", GUN, "fx_bug_splat", (792, 742, 884, 840), "white"),
    ("one", GUN, "fx_bug_boom", (916, 742, 1008, 840), "white"),
    ("one", SPELL, "fx_bug_blue", (806, 468, 906, 575), "white"),
    ("one", SPELL, "fx_bug_debris", (914, 462, 1016, 584), "white"),
    # Zombies
    ("one", ZOMBIE, "zombie_0", (548, 658, 620, 787), "white"),
    ("one", ZOMBIE, "zombie_1", (672, 658, 745, 787), "white"),
    ("one", ZOMBIE, "zombie_2", (790, 670, 885, 806), "white"),
    ("one", ZOMBIE, "zombie_3", (912, 672, 985, 802), "white"),
    ("one", ZOMBIE, "zombie_atk_0", (550, 856, 645, 982), "white"),
    ("one", ZOMBIE, "zombie_atk_1", (676, 854, 765, 982), "white"),
    ("one", ZOMBIE, "zombie_crawl", (895, 886, 1012, 982), "white"),
    ("one", ZOMBIE, "bubble_a", (549, 789, 644, 853), "white"),
    ("one", SPELL, "fx_zombie_hit", (668, 657, 768, 790), "white"),
    ("one", SPELL, "fx_zombie_green", (772, 658, 892, 805), "white"),
    ("one", SPELL, "fx_zombie_boom", (632, 852, 785, 982), "white"),
    ("one", SPELL, "fx_smoke", (792, 886, 886, 982), "white"),
    ("one", SPELL, "fx_remains", (892, 898, 1018, 982), "white"),
    # The world
    ("one", WORLD, "tile_solid", (0, 212, 42, 255), "none"),
    ("one", WORLD, "tile_q", (43, 212, 85, 255), "none"),
    ("one", WORLD, "tile_brick", (0, 470, 42, 512), "none"),
    ("one", WORLD, "tile_gold", (385, 300, 426, 341), "none"),
    ("one", WORLD, "tile_stone", (0, 664, 42, 706), "none"),
    ("one", WORLD, "ground_grass", (46, 940, 166, 962), "none"),
    ("one", WORLD, "ground_dirt", (46, 962, 166, 985), "none"),
    ("one", WORLD, "ground_rock", (172, 942, 214, 985), "none"),
    ("one", WORLD, "pipe_cap", (300, 726, 386, 768), "none"),
    ("one", WORLD, "pipe_body", (303, 772, 383, 800), "none"),
    ("one", WORLD, "coin", (302, 299, 338, 341), "sky"),
    ("one", WORLD, "mushroom", (420, 208, 482, 268), "sky"),
    ("one", WORLD, "star", (455, 402, 520, 468), "sky"),
    ("one", WORLD, "bottle", (938, 345, 1012, 466), "sky"),
]

WHITE_MIN = 205


def is_background(pixel, bg):
    r, g, b = pixel
    if bg == "white":
        return min(r, g, b) >= WHITE_MIN
    if bg == "sky":
        return (min(r, g, b) >= WHITE_MIN) or (b > r + 28 and b >= g - 6)
    return False


def key_background(image, bg):
    """Returns the image as RGBA with the connected background transparent."""
    rgba = image.convert("RGBA")
    if bg == "none":
        return rgba
    width, height = rgba.size
    pixels = rgba.load()
    seen = bytearray(width * height)
    stack = []
    for x in range(width):
        stack.append((x, 0))
        stack.append((x, height - 1))
    for y in range(height):
        stack.append((0, y))
        stack.append((width - 1, y))
    while stack:
        x, y = stack.pop()
        if x < 0 or y < 0 or x >= width or y >= height or seen[y * width + x]:
            continue
        if not is_background(pixels[x, y][:3], bg):
            continue
        seen[y * width + x] = 1
        stack.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    for y in range(height):
        for x in range(width):
            if seen[y * width + x]:
                pixels[x, y] = (0, 0, 0, 0)
    # Light fringe left by JPEG blur next to the keyed area.
    fringe = []
    for y in range(height):
        for x in range(width):
            if seen[y * width + x]:
                continue
            r, g, b, _ = pixels[x, y]
            if min(r, g, b) < 150 and not (bg == "sky" and b > r + 40):
                continue
            for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                if 0 <= nx < width and 0 <= ny < height and seen[ny * width + nx]:
                    fringe.append((x, y))
                    break
    for x, y in fringe:
        pixels[x, y] = (0, 0, 0, 0)
    return rgba


def blobs(rgba, cell=2, reach=2):
    """Connected blobs of opaque cells, each as a set of (cx, cy)."""
    width, height = rgba.size
    pixels = rgba.load()
    cols, rows = (width + cell - 1) // cell, (height + cell - 1) // cell
    occupied = set()
    for cy in range(rows):
        for cx in range(cols):
            for y in range(cy * cell, min(height, cy * cell + cell)):
                if any(pixels[x, y][3] for x in range(cx * cell, min(width, cx * cell + cell))):
                    occupied.add((cx, cy))
                    break
    seen, found = set(), []
    for start in sorted(occupied):
        if start in seen:
            continue
        seen.add(start)
        stack, members = [start], set()
        while stack:
            cx, cy = stack.pop()
            members.add((cx, cy))
            for dy in range(-reach, reach + 1):
                for dx in range(-reach, reach + 1):
                    n = (cx + dx, cy + dy)
                    if n in occupied and n not in seen:
                        seen.add(n)
                        stack.append(n)
        found.append(members)
    return found


def tall_enough(members, cell=2, minimum=40):
    """Row labels are thin; frames are not."""
    ys = [y for _, y in members]
    return (max(ys) - min(ys) + 1) * cell >= minimum


def tight(rgba, members=None, cell=2):
    """Crops to the opaque pixels (only those of the blob when given)."""
    width, height = rgba.size
    out = Image.new("RGBA", rgba.size, (0, 0, 0, 0))
    src, dst = rgba.load(), out.load()
    for y in range(height):
        for x in range(width):
            if members is not None and (x // cell, y // cell) not in members:
                continue
            if src[x, y][3]:
                dst[x, y] = src[x, y]
    box = out.getbbox()
    return out.crop(box) if box else None


def feet_anchor(sprite):
    """Anchor at the bottom, x at the centre of the lowest third of the ink."""
    width, height = sprite.size
    pixels = sprite.load()
    low = int(height * 0.7)
    total = weight = 0
    for y in range(low, height):
        for x in range(width):
            if pixels[x, y][3]:
                total += 1
                weight += x
    return (round(weight / total) if total else width // 2), height - 1


def paint_bottle_label(sprite):
    """The generated bottle says 'Non'. It is the TDD potion: relabel it."""
    draw = ImageDraw.Draw(sprite)
    width, height = sprite.size
    left, top, right, bottom = 4, 61, width - 4, 96
    draw.rectangle((left, top, right, bottom), fill=(236, 232, 200, 255))
    try:
        font = ImageFont.load_default(size=22)
    except TypeError:
        font = ImageFont.load_default()
    text = "TDD"
    box = draw.textbbox((0, 0), text, font=font)
    draw.text((left + ((right - left) - (box[2] - box[0])) // 2 - box[0],
               top + ((bottom - top) - (box[3] - box[1])) // 2 - box[1]),
              text, font=font, fill=(10, 10, 10, 255))


def cook(output):
    output.mkdir(parents=True, exist_ok=True)
    sheets = {}
    manifest = []

    def emit(name, sprite):
        if name == "bottle":
            paint_bottle_label(sprite)
        sprite.save(output / (name + ".png"))
        ax, ay = feet_anchor(sprite)
        manifest.append("%s %s.png %d %d %d %d" % (name, name, sprite.width, sprite.height, ax, ay))

    for entry in SPEC:
        kind, sheet = entry[0], entry[1]
        if sheet not in sheets:
            sheets[sheet] = Image.open(SOURCE / sheet).convert("RGB")
        if kind == "band":
            rect, names, bg = entry[2], entry[3], entry[4]
            keyed = key_background(sheets[sheet].crop(rect), bg)
            found = [b for b in blobs(keyed) if len(b) > 40 and tall_enough(b)]
            found.sort(key=lambda b: min(x for x, _ in b))
            if len(found) != len(names):
                boxes = [(rect[0] + 2 * min(x for x, _ in b), rect[1] + 2 * min(y for _, y in b),
                          rect[0] + 2 * max(x for x, _ in b), rect[1] + 2 * max(y for _, y in b))
                         for b in found]
                sys.exit("%s %s: expected %d frames, found %d: %s" % (sheet, rect, len(names), len(found), boxes))
            for name, members in zip(names, found):
                emit(name, tight(keyed, members))
        else:
            name, rect, bg = entry[2], entry[3], entry[4]
            keyed = key_background(sheets[sheet].crop(rect), bg)
            sprite = tight(keyed)
            if sprite is None:
                sys.exit("%s is empty" % name)
            emit(name, sprite)
    (output / "manifest.txt").write_text("\n".join(manifest) + "\n")
    # The box art is the title screen.
    Image.open(SOURCE / "box_art.jpeg").convert("RGB").resize((768, 768), Image.LANCZOS).save(output / "cover.png")
    return len(manifest)


def contact_sheet(output, path):
    names = [line.split()[0] for line in (output / "manifest.txt").read_text().splitlines()]
    cell, columns = 150, 10
    rows = (len(names) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * cell, rows * cell), (150, 190, 120))
    draw = ImageDraw.Draw(sheet)
    for i, name in enumerate(names):
        sprite = Image.open(output / (name + ".png"))
        sprite.thumbnail((cell - 6, cell - 22))
        x, y = (i % columns) * cell, (i // columns) * cell
        sheet.paste(sprite, (x + 3, y + 16), sprite)
        draw.text((x + 3, y + 2), name, fill=(0, 0, 0))
    sheet.save(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=OUT)
    parser.add_argument("--contact", type=Path, help="also write a contact sheet PNG")
    args = parser.parse_args()
    count = cook(args.output)
    if args.contact:
        contact_sheet(args.output, args.contact)
    print("cooked %d sprites into %s" % (count, args.output))
