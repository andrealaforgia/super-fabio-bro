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
MONK_CROSS = "monk_cross.jpeg"
RETRO = "retro_characters.png"
BUBBLES = "bubbles.png"
CHEF_HAPPY = "chef_happy.jpeg"
CHEF_ANGRY = "chef_angry.jpeg"
GORILLA = "gorilla.jpeg"
QBLOCK = "question_block.png"
DEMON = "demon.jpeg"
VEGETABLES = "vegetables.jpeg"
DRUM_KIT = "drum_kit.png"
DRUMMER = "drummer_frantic.png"
HELICOPTER = "helicopter.jpeg"
FABIO_HELICOPTER = "fabio_helicopter.jpeg"
SKEWER = "skewer.jpeg"
BUBBLE_GUN = "bubble_gun.png"
FABIO_BUBBLE_GUN = "fabio_bubble_gun.png"

# kind "band": every blob in the rectangle, left to right, named in order.
# kind "one": the rectangle is one sprite.
# bg "white", "paper", "cutout", "light", "sky", "magenta" and "vivid" (pure magenta only)
# are keyed out, "alpha" keeps
# the sheet's own transparency ("cutout" also clears paper
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
    # The hooded monk, from the second row of his own sheet: hands joined, then the cross
    # comes out and is held up, and finally flashes red.
    ("one", MONK_CROSS, "monk_cross_0", (50, 380, 250, 640), "checker", "fixed", "clean", "scale=0.2795"),
    ("one", MONK_CROSS, "monk_cross_1", (300, 380, 500, 640), "checker", "fixed", "clean", "scale=0.2795"),
    ("one", MONK_CROSS, "monk_cross_2", (555, 380, 775, 640), "checker", "fixed", "clean", "scale=0.2795"),
    ("one", MONK_CROSS, "monk_cross_3", (800, 380, 1020, 640), "checker", "fixed", "clean", "scale=0.2795"),
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
    ("one", BUBBLES, "bubble_chef", (260, 0, 450, 130), "white"),
    # The chef: two happy frames, then the angry swing, then the blast (0.261, ten
    # per cent under the 0.29 he was first cut at).
    ("one", CHEF_HAPPY, "chef_happy_0", (25, 18, 305, 315), "checker", "fixed", "scale=0.261"),
    ("one", CHEF_HAPPY, "chef_happy_1", (10, 355, 268, 652), "checker", "fixed", "scale=0.261"),
    ("one", CHEF_HAPPY, "chef_boom_0", (270, 355, 545, 652), "checker", "fixed", "scale=0.261"),
    ("one", CHEF_HAPPY, "chef_boom_1", (585, 355, 955, 652), "checker", "fixed", "scale=0.261"),
    ("band", CHEF_ANGRY, (232, 30, 1015, 320), ["chef_attack_%d" % i for i in range(4)], "checker", "scale=0.261"),

    # The gorilla, facing right: standing, roaring, beating his chest and throwing
    # a banana; and the banana itself. Every frame is the same 400-pixel band of its
    # cell down to his feet, kept whole and anchored at its middle ("centre"), so he
    # stays put from frame to frame. They are drawn small, so they are cooked at half
    # size ("half").
    ("one", GORILLA, "gorilla_idle", (64, 24, 464, 399), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_roar_0", (64, 453, 464, 835), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_roar_1", (574, 453, 974, 835), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_roar_2", (1084, 453, 1484, 835), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_roar_3", (1595, 453, 1995, 835), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_beat_0", (64, 883, 464, 1263), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_beat_1", (1595, 883, 1995, 1263), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_throw_0", (64, 1313, 464, 1736), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_throw_1", (574, 1313, 974, 1736), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_throw_2", (1084, 1313, 1484, 1736), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "gorilla_throw_3", (1595, 1313, 1995, 1736), "magenta", "fixed", "centre", "half"),
    ("one", GORILLA, "banana", (24, 1779, 321, 2023), "magenta"),
    # The demon who rises from the monk, facing right: standing, casting a fireball,
    # dying and dissolving; and the fireball. Frames are whole cells down to his
    # feet, anchored at their middle and cooked at half size, like the gorilla's.
    ("one", DEMON, "demon_stand_0", (21, 21, 495, 403), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_stand_1", (531, 21, 1005, 403), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_stand_2", (1041, 21, 1515, 403), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_stand_3", (1552, 21, 2026, 403), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_cast_0", (21, 450, 495, 841), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_cast_1", (531, 450, 1005, 841), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_cast_2", (1041, 450, 1515, 841), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_cast_3", (1552, 450, 2026, 841), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_0", (21, 880, 495, 1274), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_1", (531, 880, 1005, 1274), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_2", (1041, 880, 1515, 1274), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_3", (1552, 880, 2026, 1274), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_4", (21, 1310, 495, 1740), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_5", (531, 1310, 1005, 1740), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_6", (1041, 1310, 1515, 1740), "magenta", "fixed", "centre", "half"),
    ("one", DEMON, "demon_death_7", (1552, 1310, 2026, 1740), "magenta", "fixed", "centre", "half"),
    # The lamb skewer Fabio shoots from the helicopter: the first "thrown projectile"
    # frame, tilted up to the right at 45 degrees. The game turns it as it flies.
    ("one", SKEWER, "skewer", (8, 618, 132, 740), "white", "scale=0.3"),
    ("one", DEMON, "demon_fireball", (930, 640, 1000, 705), "magenta"),
    # Fabio's drum kit: at rest (the first kit of the first sheet), then the eight frames of the third
    # row of the frantic sheet, in which he plays it flat out. The cells are kept whole so the
    # kit stays put from frame to frame; their bottoms sit on the kit's feet, and the scale makes
    # kit and drummer about as tall as Fabio.
    # The soap bubble shotgun: Fabio carrying it (his sheet's first six rows; the
    # sheet's Fabio is a touch taller than the old ones, so he is shrunk to match),
    # the gun itself for the pickup, and the big bubble cut round as a circle.
    ("band", FABIO_BUBBLE_GUN, (0, 30, 640, 175), ["fab_bub_idle_%d" % i for i in range(5)], "light", "scale=0.9"),
    ("band", FABIO_BUBBLE_GUN, (0, 195, 1254, 340), ["fab_bub_walk_%d" % i for i in range(8)], "light", "scale=0.9"),
    ("band", FABIO_BUBBLE_GUN, (0, 360, 1254, 500), ["fab_bub_run_%d" % i for i in range(8)], "light", "scale=0.9"),
    ("band", FABIO_BUBBLE_GUN, (0, 500, 1254, 650), ["fab_bub_jump_%d" % i for i in range(5)], "light", "scale=0.9"),
    ("band", FABIO_BUBBLE_GUN, (0, 650, 1254, 775), ["fab_bub_aim_%d" % i for i in range(5)], "light", "scale=0.9"),
    ("band", FABIO_BUBBLE_GUN, (0, 790, 1254, 910), ["fab_bub_shoot_%d" % i for i in range(8)], "light", "scale=0.9"),
    ("one", BUBBLE_GUN, "bubble_gun_0", (30, 140, 170, 240), "light"),
    ("bubble", BUBBLE_GUN, "bubble_big", (1035, 936, 1229, 1118), "none"),
    ("one", DRUM_KIT, "drums_rest", (14, 66, 162, 196), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_0", (5, 520, 157, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_1", (162, 520, 314, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_2", (318, 520, 470, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_3", (474, 488, 626, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_4", (630, 488, 782, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_5", (786, 488, 938, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_6", (942, 488, 1094, 686), "light", "fixed", "scale=0.44"),
    ("one", DRUMMER, "drums_play_7", (1097, 488, 1249, 686), "light", "fixed", "scale=0.44"),
    # The one helicopter. At rest it is the first "idle hovering" frame of its own
    # sheet. With Fabio in it, the three rotor-spin frames of the second row of
    # the sheet with him inside, each a cell of the same size so the rotor and the
    # tail stay put from frame to frame (anchored at the middle of the skids' line),
    # cooked at 0.6 so the game draws them 1:1. It blows up in the seven frames of
    # the "exploding" row, equal cells centred on each blast.
    ("one", HELICOPTER, "heli_rest", (12, 40, 166, 152), "white", "scale=0.6"),
    ("one", FABIO_HELICOPTER, "fab_heli_0", (12, 186, 164, 299), "white", "fixed", "centre", "scale=0.6"),
    ("one", FABIO_HELICOPTER, "fab_heli_1", (175, 186, 327, 299), "white", "fixed", "centre", "scale=0.6"),
    ("one", FABIO_HELICOPTER, "fab_heli_2", (339, 186, 491, 299), "white", "fixed", "centre", "scale=0.6"),
    ("one", FABIO_HELICOPTER, "heli_boom_0", (11, 740, 131, 846), "white", "fixed", "centre"),
    ("one", FABIO_HELICOPTER, "heli_boom_1", (137, 740, 257, 846), "white", "fixed", "centre"),
    ("one", FABIO_HELICOPTER, "heli_boom_2", (263, 740, 383, 846), "white", "fixed", "centre"),
    ("one", FABIO_HELICOPTER, "heli_boom_3", (389, 740, 509, 846), "white", "fixed", "centre"),
    ("one", FABIO_HELICOPTER, "heli_boom_4", (515, 740, 635, 846), "cutout", "fixed", "centre"),
    ("one", FABIO_HELICOPTER, "heli_boom_5", (640, 740, 760, 846), "cutout", "fixed", "centre"),
    ("one", FABIO_HELICOPTER, "heli_boom_6", (765, 740, 885, 846), "cutout", "fixed", "centre"),
    # The chef's vegetables, flung out when he bursts: carrots, potatoes, onions,
    # broccoli, tomatoes, peppers and slices, one per cell, read left to right.
    ("one", VEGETABLES, "vegetable_0", (67, 65, 491, 489), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_1", (563, 65, 987, 489), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_2", (1060, 65, 1484, 489), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_3", (1556, 65, 1980, 489), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_4", (67, 562, 491, 986), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_5", (563, 562, 987, 986), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_6", (1060, 562, 1484, 986), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_7", (1556, 562, 1980, 986), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_8", (67, 1060, 491, 1484), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_9", (563, 1060, 987, 1484), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_10", (1060, 1060, 1484, 1484), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_11", (1556, 1060, 1980, 1484), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_12", (67, 1559, 491, 1983), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_13", (563, 1559, 987, 1983), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_14", (1060, 1559, 1484, 1983), "vivid", "scale=0.15"),
    ("one", VEGETABLES, "vegetable_15", (1556, 1559, 1980, 1983), "vivid", "scale=0.15"),
    # The "?" block turning round: the yellow column, sixteen frames, each kept a
    # whole 32-pixel square so the narrow ones are not stretched.
    # The question block turns round (16 frames); one column per colour, yellow first.
    *[("one", QBLOCK, "%s_%d" % (name, row), (32 * col, 32 * row, 32 * col + 32, 32 * row + 32),
       "alpha", "fixed")
      for col, name in enumerate(["tile_q", "tile_q_blue", "tile_q_grey", "tile_q_orange"])
      for row in range(16)],
    # The world
    ("one", WORLD, "tile_solid", (0, 212, 42, 255), "none"),
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
    if bg == "checker":  # the baked grey chequerboard: neutral, mid-light, never white or ink
        return max(r, g, b) - min(r, g, b) <= 14 and 105 <= min(r, g, b) <= 225
    if bg == "light":  # the drummer's pale grey sheet: neutral and bright, but not paper white
        return max(r, g, b) - min(r, g, b) <= 10 and min(r, g, b) >= 205
    if bg == "sky":
        return (min(r, g, b) >= WHITE_MIN) or (b > r + 28 and b >= g - 6)
    if bg == "magenta":  # the gorilla's backdrop; his browns and the banana have little blue
        return r > g + 40 and b > g + 40
    if bg == "vivid":  # pure magenta only, so a red onion's purple survives
        return r > 200 and b > 200 and g < 90
    return False


def key_background(image, bg):
    """Returns the image as RGBA with the connected background transparent."""
    rgba = image.convert("RGBA")
    if bg in ("none", "alpha"):
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
            elif bg == "vivid" and is_background(pixels[x, y][:3], bg):
                pixels[x, y] = (0, 0, 0, 0)  # backdrop showing through a gap, as between stems
    # Light fringe left by JPEG blur next to the keyed area.
    fringe = []
    for y in range(height):
        for x in range(width):
            if seen[y * width + x]:
                continue
            r, g, b, _ = pixels[x, y]
            if bg == "checker":
                continue
            pinkish = bg == "vivid" and r > g + 60 and b > g + 60  # the blurred edge of the backdrop
            if (min(r, g, b) < (225 if bg in ("paper", "cutout") else 150) and not pinkish
                    and not (bg == "sky" and b > r + 40)):
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


def wide_enough(members, cell=2, minimum=24):
    xs = [x for x, _ in members]
    return (max(xs) - min(xs) + 1) * cell >= minimum


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


def clean_edges(sprite, min_area=120, thin=14):
    """Drops slivers of the neighbouring frame left against the cell's sides
    (small blobs touching the left or right edge), and the pale halo the
    keying leaves around the outline."""
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
            xs = [x for x, _ in members]
            at_side = min(xs) == 0 or max(xs) == width - 1
            if at_side and (len(members) < min_area or max(xs) - min(xs) < thin):
                for x, y in members:
                    pixels[x, y] = (0, 0, 0, 0)
    # Background trapped inside a curl of the pose: a big patch of near-white
    # (the eyes are far smaller).
    paler = lambda r, g, b: min(r, g, b) >= 215 and max(r, g, b) - min(r, g, b) < 30
    seen = bytearray(width * height)
    for y0 in range(height):
        for x0 in range(width):
            if seen[y0 * width + x0] or not pixels[x0, y0][3] or not paler(*pixels[x0, y0][:3]):
                continue
            seen[y0 * width + x0] = 1
            stack, members = [(x0, y0)], []
            while stack:
                x, y = stack.pop()
                members.append((x, y))
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if (0 <= nx < width and 0 <= ny < height and not seen[ny * width + nx]
                            and pixels[nx, ny][3] and paler(*pixels[nx, ny][:3])):
                        seen[ny * width + nx] = 1
                        stack.append((nx, ny))
            if len(members) >= 100:
                for x, y in members:
                    pixels[x, y] = (0, 0, 0, 0)
    # Light, grey-ish pixels touching the transparent area are halo, not ink.
    for _ in range(2):
        halo = []
        for y in range(height):
            for x in range(width):
                r, g, b, a = pixels[x, y]
                if not a or max(r, g, b) - min(r, g, b) > 45 or (r + g + b) / 3 < 150:
                    continue
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if not (0 <= nx < width and 0 <= ny < height) or not pixels[nx, ny][3]:
                        halo.append((x, y))
                        break
        for x, y in halo:
            pixels[x, y] = (0, 0, 0, 0)
    return sprite


def bake_scale(sprite, factor):
    """Enlarges the sprite to the size it is drawn at, with a sharp filter and
    hard edges, so the game draws it 1:1 instead of stretching it blurry."""
    size = (round(sprite.width * factor), round(sprite.height * factor))
    big = sprite.resize(size, Image.LANCZOS)
    if factor < 1.0:
        return big  # shrunk, so the edges stay anti-aliased
    pixels = big.load()
    for y in range(big.height):
        for x in range(big.width):
            r, g, b, a = pixels[x, y]
            pixels[x, y] = (r, g, b, 255) if a >= 128 else (0, 0, 0, 0)
    return big


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


def baked(sprite, options):
    """Applies a "scale=F" option: the sprite is resized once, so the game draws it 1:1."""
    for option in options:
        if option.startswith("scale="):
            return bake_scale(sprite, float(option[6:]))
    return sprite


def soap_bubble(crop):
    """A soap bubble cut round out of its cell: a clear body, a solid blue rim and
    bright glints, so the game can draw it any size over whatever is behind."""
    size = min(crop.size)
    radius = size / 2.0 - 1.5
    sprite = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    src, out = crop.convert("RGB").load(), sprite.load()
    for y in range(size):
        for x in range(size):
            if (x - size / 2.0 + 0.5) ** 2 + (y - size / 2.0 + 0.5) ** 2 > radius ** 2:
                continue
            r, g, b = src[x, y]
            alpha = 255 if r < 110 else (205 if min(r, g, b) > 245 else 105)
            out[x, y] = (r, g, b, alpha)
    return sprite


def cook(output):
    output.mkdir(parents=True, exist_ok=True)
    sheets = {}
    manifest = []

    def emit(name, sprite, centre=False):
        if name == "bottle":
            paint_bottle_label(sprite)
        sprite.save(output / (name + ".png"))
        ax, ay = (sprite.width // 2, sprite.height - 1) if centre else feet_anchor(sprite)
        manifest.append("%s %s.png %d %d %d %d" % (name, name, sprite.width, sprite.height, ax, ay))

    for entry in SPEC:
        kind, sheet = entry[0], entry[1]
        if sheet not in sheets:
            sheets[sheet] = Image.open(SOURCE / sheet).convert("RGBA" if entry[4] == "alpha" else "RGB")
        if kind == "bubble":
            emit(entry[2], soap_bubble(sheets[sheet].crop(entry[3])), True)
            continue
        if kind == "band":
            rect, names, bg = entry[2], entry[3], entry[4]
            keyed = key_background(sheets[sheet].crop(rect), bg)
            found = [b for b in blobs(keyed) if len(b) > 40 and tall_enough(b) and wide_enough(b)]
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
                emit(name, baked(sprite, entry[5:]))
        else:
            name, rect, bg = entry[2], entry[3], entry[4]
            keyed = key_background(sheets[sheet].crop(rect), bg)
            sprite = keyed if "fixed" in entry[5:] else tight(keyed)
            if sprite is None:
                sys.exit("%s is empty" % name)
            if "clean" in entry[5:]:
                sprite = clean_edges(sprite)
            sprite = baked(sprite, entry[5:])
            if "half" in entry[5:]:
                sprite = sprite.resize((sprite.width // 2, sprite.height // 2), Image.LANCZOS)
            emit(name, sprite, "centre" in entry[5:])
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
