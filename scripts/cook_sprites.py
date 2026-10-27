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
BOOK_READER = "book_reader.jpeg"
MONK = "monk.png"
RETRO = "retro_characters.png"
BUBBLES = "bubbles.png"

# kind "band": every blob in the rectangle, left to right, named in order.
# kind "one": the rectangle is one sprite.
# bg "white", "paper", "cutout" and "sky" are keyed out ("cutout" also clears paper
# seen through gaps in the pose, which "paper" keeps for bright blast cores), "none" keeps the rectangle opaque.
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
    # The book reader, reading Fabio's book. "notext" erases the
    # "HAHAHA!" lettering the generator drew beside the laughing frames.
    ("band", BOOK_READER, (0, 322, 1024, 445),
     ["reader_read_0", "reader_read_1", "reader_read_2", "reader_snort_0", "reader_snort_1"], "white"),
    ("band", BOOK_READER, (0, 462, 1024, 585), ["reader_snigger_%d" % i for i in range(7)], "white"),
    ("band", BOOK_READER, (0, 596, 1024, 720), ["reader_laugh_%d" % i for i in range(8)], "white", "notext"),
    ("band", BOOK_READER, (0, 735, 500, 850), ["reader_pop_%d" % i for i in range(4)], "white"),
    ("band", BOOK_READER, (30, 862, 380, 985), ["reader_pop_%d" % i for i in range(4, 7)], "white"),
    ("one", BOOK_READER, "reader_flash", (388, 858, 522, 980), "white"),
    ("one", BOOK_READER, "reader_boom", (534, 740, 874, 982), "white"),
    ("one", BOOK_READER, "reader_remains", (898, 745, 1014, 852), "white"),
    # The monk who will not pair, and the sixteen frames of his explosion.
    ("one", MONK, "monk_boom_0", (204, 30, 434, 345), "paper"),
    ("one", MONK, "monk_boom_1", (434, 30, 672, 345), "paper"),
    ("one", MONK, "monk_boom_2", (672, 30, 944, 345), "paper"),
    ("one", MONK, "monk_boom_3", (944, 30, 1229, 345), "paper"),
    ("one", MONK, "monk_boom_4", (1229, 30, 1505, 345), "paper"),
    ("one", MONK, "monk_boom_5", (1505, 30, 1765, 345), "paper"),
    ("one", MONK, "monk_boom_6", (1765, 30, 2043, 345), "paper"),
    ("one", MONK, "monk_boom_7", (0, 415, 230, 690), "paper"),
    ("one", MONK, "monk_boom_8", (230, 415, 454, 690), "paper"),
    ("one", MONK, "monk_boom_9", (454, 415, 688, 690), "paper"),
    ("one", MONK, "monk_boom_10", (688, 415, 944, 690), "paper"),
    ("one", MONK, "monk_boom_11", (944, 415, 1224, 690), "paper"),
    ("one", MONK, "monk_boom_12", (1224, 415, 1540, 690), "paper"),
    ("one", MONK, "monk_boom_13", (1540, 415, 1800, 690), "paper"),
    ("one", MONK, "monk_boom_14", (1800, 415, 2043, 690), "paper"),
    # The hooded monk standing (his walk cycle, facing right) and the skeleton who
    # loves PRs, walking and vanishing. The skeleton's vanishing frames keep their whole cell ("fixed") so
    # the dust stays where it fell.
    ("one", RETRO, "monk_idle", (10, 85, 127, 300), "cutout"),
    ("one", RETRO, "skeleton_vanish_0", (1064, 455, 1161, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_1", (1161, 455, 1254, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_2", (1254, 455, 1350, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_3", (1350, 455, 1439, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_4", (1439, 455, 1523, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_5", (1523, 455, 1605, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_6", (1605, 455, 1685, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_7", (1685, 455, 1766, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_8", (1766, 455, 1853, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_9", (1853, 455, 1916, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_vanish_10", (1916, 455, 1983, 692), "cutout", "fixed"),
    # The skeleton's walk cycle, cells kept whole so the stride stays put.
    ("one", RETRO, "skeleton_walk_0", (12, 455, 124, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_1", (124, 455, 239, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_2", (239, 455, 356, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_3", (356, 455, 471, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_4", (471, 455, 591, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_5", (591, 455, 703, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_6", (703, 455, 816, 692), "cutout", "fixed"),
    ("one", RETRO, "skeleton_walk_7", (816, 455, 945, 692), "cutout", "fixed"),
    # Their speech bubbles, made by scripts/make_bubbles.py in the zombies' style.
    ("one", BUBBLES, "bubble_monk", (0, 0, 130, 100), "white"),
    ("one", BUBBLES, "bubble_skeleton", (130, 0, 260, 100), "white"),
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
PAPER_MIN = 247


def is_background(pixel, bg):
    r, g, b = pixel
    if bg == "white":
        return min(r, g, b) >= WHITE_MIN
    if bg in ("paper", "cutout"):  # a clean sheet: only near-white goes, so pale smoke stays
        return min(r, g, b) >= PAPER_MIN
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
            elif bg == "cutout" and min(pixels[x, y][:3]) >= PAPER_MIN:
                pixels[x, y] = (0, 0, 0, 0)  # paper showing through a gap in the pose
    # Light fringe left by JPEG blur next to the keyed area.
    fringe = []
    for y in range(height):
        for x in range(width):
            if seen[y * width + x]:
                continue
            r, g, b, _ = pixels[x, y]
            if min(r, g, b) < (225 if bg in ("paper", "cutout") else 150) and not (bg == "sky" and b > r + 40):
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


def erase_lettering(sprite, tallest=24, smallest_area=400):
    """Removes the small separate bits of ink (letters, exclamation marks)
    from a sprite, keeping the figure and anything big, like a dropped book."""
    width, height = sprite.size
    pixels = sprite.load()
    seen = bytearray(width * height)
    for y0 in range(height):
        for x0 in range(width):
            if seen[y0 * width + x0] or not pixels[x0, y0][3]:
                continue
            seen[y0 * width + x0] = 1
            stack, members = [(x0, y0)], []
            while stack:
                x, y = stack.pop()
                members.append((x, y))
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        nx, ny = x + dx, y + dy
                        if (0 <= nx < width and 0 <= ny < height and not seen[ny * width + nx]
                                and pixels[nx, ny][3]):
                            seen[ny * width + nx] = 1
                            stack.append((nx, ny))
            top = min(y for _, y in members)
            bottom = max(y for _, y in members)
            if bottom - top + 1 < tallest and len(members) < smallest_area:
                for x, y in members:
                    pixels[x, y] = (0, 0, 0, 0)
    box = sprite.getbbox()
    return sprite.crop(box) if box else sprite


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
                sprite = tight(keyed, members)
                if "notext" in entry[5:]:
                    sprite = erase_lettering(sprite)
                emit(name, sprite)
        else:
            name, rect, bg = entry[2], entry[3], entry[4]
            keyed = key_background(sheets[sheet].crop(rect), bg)
            sprite = keyed if "fixed" in entry[5:] else tight(keyed)
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


BOOK_READER_SHEET = [
    ["reader_read_0", "reader_read_1", "reader_read_2", "reader_snort_0", "reader_snort_1"],
    ["reader_snigger_%d" % i for i in range(7)],
    ["reader_laugh_%d" % i for i in range(8)],
    ["reader_pop_%d" % i for i in range(7)] + ["reader_flash", "reader_boom", "reader_remains"],
]


def book_reader_sheet(output, path, pad=10):
    """One transparent PNG with every book reader frame, a row per animation
    (reading, sniggering, laughing, bursting) and no lettering at all."""
    rows = [[Image.open(output / (name + ".png")) for name in row] for row in BOOK_READER_SHEET]
    width = max(sum(im.width + pad for im in row) + pad for row in rows)
    height = sum(max(im.height for im in row) + pad for row in rows) + pad
    sheet = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    y = pad
    for row in rows:
        tallest = max(im.height for im in row)
        x = pad
        for im in row:
            sheet.paste(im, (x, y + tallest - im.height), im)
            x += im.width + pad
        y += tallest + pad
    sheet.save(path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=OUT)
    parser.add_argument("--book-reader-sheet", type=Path, help="also write the book reader sprite sheet PNG")
    parser.add_argument("--contact", type=Path, help="also write a contact sheet PNG")
    args = parser.parse_args()
    count = cook(args.output)
    if args.contact:
        contact_sheet(args.output, args.contact)
    if args.book_reader_sheet:
        book_reader_sheet(args.output, args.book_reader_sheet)
    print("cooked %d sprites into %s" % (count, args.output))
