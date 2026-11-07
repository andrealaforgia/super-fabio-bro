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
SLOP = "slop_monster.png"
FABIO_JEDI = "fabio_jedi.png"
JEDI_ITEMS = "jedi_items.png"
BIOME_GROUND = "biome_ground.png"
BIOME_DECOR = "biome_decor.png"
ALLY_ITEMS = "ally_items.png"
ALLY_ANDREA = "ally_andrea.png"
ALLY_ALESSANDRO = "ally_alessandro.png"
ALLY_GIULIA = "ally_giulia.png"
ALLY_GIACOMO = "ally_giacomo.png"
GIACOMO_LOW = "giacomo_low.png"
BUBBLE_GUN = "bubble_gun.png"
FABIO_BUBBLE_GUN = "fabio_bubble_gun.png"
FABIO_PARACHUTE = "fabio_parachute.png"
CITY_PROPS = "city_props.png"
THAI_SHOP = "thai_shop.png"
BULL = "bull.png"
LANDMARKS_TALL = "landmarks_tall.png"
LANDMARKS_WIDE = "landmarks_wide.png"
COLOSSO = "colosso.png"

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
    # Fabio under the parachute (cooked at the size it is drawn, so the game draws it 1:1), the
    # cells kept whole and centred on him, their bottoms on his feet.
    ("one", FABIO_PARACHUTE, "chute_fall_0", (205, 8, 475, 282), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_fall_1", (500, 8, 770, 282), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_fall_2", (795, 8, 1065, 282), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_shoot_0", (205, 300, 475, 570), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_shoot_1", (500, 300, 810, 570), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_shoot_2", (795, 300, 1105, 570), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_open_0", (262, 590, 408, 860), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_open_1", (495, 590, 765, 860), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_open_2", (795, 590, 1065, 860), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_land_0", (165, 890, 340, 1150), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_land_1", (390, 890, 610, 1150), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_land_2", (650, 890, 890, 1150), "light", "fixed", "centre", "scale=0.37"),
    ("one", FABIO_PARACHUTE, "chute_land_3", (970, 890, 1120, 1150), "light", "fixed", "centre", "scale=0.37"),
    # The city of Sulmona: Ovid, the aqueduct, the tap, the water, the cars, the vases, the fountain.
    ("one", CITY_PROPS, "city_ovid", (322, 0, 480, 300), "light", "scale=0.5"),
    ("one", CITY_PROPS, "city_aqueduct", (360, 300, 895, 522), "light", "scale=0.755"),
    ("one", CITY_PROPS, "city_tap_0", (326, 526, 408, 630), "light", "scale=0.55"),
    ("one", CITY_PROPS, "city_tap_1", (498, 526, 582, 630), "light", "scale=0.55"),
    ("one", CITY_PROPS, "city_tap_2", (668, 526, 756, 630), "light", "scale=0.55"),
    ("one", CITY_PROPS, "city_tap_3", (840, 526, 930, 630), "light", "scale=0.55"),
    ("one", CITY_PROPS, "city_water_0", (340, 638, 400, 766), "light"),
    ("one", CITY_PROPS, "city_water_1", (510, 638, 570, 766), "light"),
    ("one", CITY_PROPS, "city_water_2", (680, 638, 740, 766), "light"),
    ("one", CITY_PROPS, "city_water_3", (852, 638, 912, 766), "light"),
    ("one", CITY_PROPS, "city_splash_0", (318, 786, 402, 840), "light", "scale=0.7"),
    ("one", CITY_PROPS, "city_splash_1", (486, 780, 588, 840), "light", "scale=0.7"),
    ("one", CITY_PROPS, "city_splash_2", (656, 776, 774, 840), "light", "scale=0.7"),
    ("one", CITY_PROPS, "city_splash_3", (828, 790, 952, 840), "light", "scale=0.7"),
    ("one", CITY_PROPS, "city_car_0", (150, 850, 378, 958), "light", "scale=0.5"),
    ("one", CITY_PROPS, "city_car_1", (414, 850, 610, 958), "light", "scale=0.5"),
    ("one", CITY_PROPS, "city_car_2", (648, 850, 878, 958), "light", "scale=0.5"),
    ("one", CITY_PROPS, "city_car_3", (914, 850, 1166, 958), "light", "scale=0.5"),
    ("one", CITY_PROPS, "city_vase_0", (264, 960, 378, 1090), "light", "scale=0.3"),
    ("one", CITY_PROPS, "city_vase_1", (520, 960, 650, 1090), "light", "scale=0.3"),
    ("one", CITY_PROPS, "city_vase_2", (740, 978, 964, 1092), "light", "scale=0.3"),
    ("one", CITY_PROPS, "city_fountain_0", (382, 1082, 548, 1192), "light", "scale=0.6"),
    ("one", CITY_PROPS, "city_fountain_1", (652, 1082, 818, 1192), "light", "scale=0.6"),
    # The Bullring bull: running, the statue, coming alive, dust.
    ("one", BULL, "bull_run_0", (205, 90, 465, 215), "light", "scale=0.72"),
    ("one", BULL, "bull_run_1", (475, 90, 715, 215), "light", "scale=0.72"),
    ("one", BULL, "bull_run_2", (730, 90, 970, 215), "light", "scale=0.72"),
    ("one", BULL, "bull_run_3", (985, 90, 1230, 215), "light", "scale=0.72"),
    ("one", BULL, "bull_run_4", (220, 262, 445, 392), "light", "scale=0.72"),
    ("one", BULL, "bull_run_5", (465, 262, 705, 392), "light", "scale=0.72"),
    ("one", BULL, "bull_run_6", (720, 262, 965, 392), "light", "scale=0.72"),
    ("one", BULL, "bull_run_7", (975, 262, 1230, 392), "light", "scale=0.72"),
    ("one", BULL, "bull_statue_0", (235, 462, 580, 665), "light", "scale=0.42"),
    ("one", BULL, "bull_appear_0", (200, 712, 410, 895), "light", "scale=0.72"),
    ("one", BULL, "bull_appear_1", (425, 725, 655, 895), "light", "scale=0.72"),
    ("one", BULL, "bull_appear_2", (655, 700, 925, 895), "light", "scale=0.72"),
    ("one", BULL, "bull_appear_3", (930, 745, 1235, 895), "light", "scale=0.72"),
    ("one", BULL, "bull_dust_0", (180, 955, 325, 1100), "light", "scale=0.6"),
    ("one", BULL, "bull_dust_1", (380, 975, 570, 1100), "light", "scale=0.6"),
    ("one", BULL, "bull_dust_2", (620, 950, 930, 1105), "light", "scale=0.6"),
    ("one", BULL, "bull_dust_3", (980, 1000, 1195, 1100), "light", "scale=0.6"),
    # The cities' landmarks, drawn to size by the renderer.
    ("one", LANDMARKS_TALL, "lm_bigben_0", (110, 500, 242, 972), "light", "scale=0.75"),
    ("one", LANDMARKS_TALL, "lm_rotunda_0", (322, 512, 478, 972), "light", "scale=0.75"),
    ("one", COLOSSO, "lm_colosso_0", (520, 525, 748, 1018), "light", "scale=0.7"),
    ("one", LANDMARKS_TALL, "lm_pod_0", (788, 850, 960, 972), "light"),
    ("one", LANDMARKS_TALL, "lm_hub_0", (1000, 820, 1156, 972), "light"),
    ("one", LANDMARKS_WIDE, "lm_duomo_0", (460, 80, 805, 362), "light"),
    ("one", LANDMARKS_WIDE, "lm_castello_0", (452, 485, 810, 625), "light"),
    ("one", LANDMARKS_WIDE, "lm_abbey_0", (520, 712, 745, 870), "light"),
    ("one", LANDMARKS_WIDE, "lm_tin_0", (175, 1015, 298, 1120), "light"),
    ("one", LANDMARKS_WIDE, "lm_bus_0", (515, 1015, 722, 1127), "light"),
    ("one", LANDMARKS_WIDE, "lm_tram_0", (918, 1005, 1130, 1128), "light"),
    # The Thai massage shops: the masseuses at their windows, the neon sign, a lantern, a flower box.
    ("one", THAI_SHOP, "thai_window_0", (38, 60, 220, 266), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_1", (245, 60, 427, 266), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_2", (450, 60, 632, 266), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_3", (654, 60, 836, 266), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_4", (858, 60, 1040, 266), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_5", (1060, 60, 1244, 266), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_b_0", (38, 334, 220, 548), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_b_1", (245, 334, 427, 548), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_b_2", (450, 334, 632, 548), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_b_3", (654, 334, 836, 548), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_b_4", (858, 334, 1040, 548), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_window_b_5", (1060, 334, 1244, 548), "light", "fixed", "scale=0.5"),
    ("one", THAI_SHOP, "thai_sign_0", (30, 612, 304, 768), "light", "fixed", "scale=0.42"),
    ("one", THAI_SHOP, "thai_sign_1", (344, 612, 614, 768), "light", "fixed", "scale=0.42"),
    ("one", THAI_SHOP, "thai_sign_2", (652, 612, 922, 768), "light", "fixed", "scale=0.42"),
    ("one", THAI_SHOP, "thai_sign_3", (960, 612, 1232, 768), "light", "fixed", "scale=0.42"),
    ("one", THAI_SHOP, "thai_lantern_0", (254, 814, 422, 990), "light", "fixed", "scale=0.3"),
    ("one", THAI_SHOP, "thai_lantern_1", (554, 814, 686, 990), "light", "fixed", "scale=0.3"),
    ("one", THAI_SHOP, "thai_lantern_2", (818, 814, 984, 990), "light", "fixed", "scale=0.3"),
    ("one", THAI_SHOP, "thai_flowers_0", (326, 1020, 582, 1166), "light", "scale=0.36"),
    ("one", THAI_SHOP, "thai_flowers_1", (668, 1020, 920, 1166), "light", "scale=0.36"),
    # The AI slop monster, who throws slop: idle, windup, throw, hurt, death, the slop ball in
    # flight and splatting, and the pieces he bursts into. "main" keeps only the biggest blob.
    ("one", SLOP, "slop_idle_0", (150, 0, 392, 170), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_idle_1", (410, 0, 650, 170), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_idle_2", (675, 0, 930, 170), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_idle_3", (945, 0, 1200, 170), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_windup_0", (165, 175, 390, 352), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_windup_1", (425, 175, 635, 352), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_windup_2", (665, 175, 875, 352), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_windup_3", (940, 175, 1165, 352), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_throw_0", (150, 355, 370, 540), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_throw_1", (395, 355, 655, 540), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_throw_2", (655, 355, 975, 540), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_throw_3", (925, 355, 1212, 540), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_hurt_0", (125, 548, 385, 708), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_hurt_1", (430, 548, 650, 708), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_hurt_2", (690, 548, 950, 708), "light", "main", "scale=0.45"),
    ("one", SLOP, "slop_death_0", (55, 712, 245, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_1", (246, 712, 395, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_2", (396, 712, 535, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_3", (536, 712, 675, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_4", (678, 712, 820, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_5", (822, 712, 960, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_6", (970, 712, 1090, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_death_7", (1100, 712, 1225, 885), "light", "scale=0.45"),
    ("one", SLOP, "slop_ball_0", (160, 898, 285, 1002), "light", "scale=0.3"),
    ("one", SLOP, "slop_ball_1", (325, 898, 450, 1002), "light", "scale=0.3"),
    ("one", SLOP, "slop_ball_2", (495, 898, 620, 1002), "light", "scale=0.3"),
    ("one", SLOP, "slop_ball_3", (668, 898, 790, 1002), "light", "scale=0.3"),
    ("one", SLOP, "slop_ball_4", (845, 898, 960, 1002), "light", "scale=0.3"),
    ("one", SLOP, "slop_ball_5", (1010, 898, 1145, 1002), "light", "scale=0.3"),
    ("one", SLOP, "slop_splat_0", (180, 1000, 290, 1100), "light", "scale=0.45"),
    ("one", SLOP, "slop_splat_1", (330, 1000, 465, 1100), "light", "scale=0.45"),
    ("one", SLOP, "slop_splat_2", (480, 1000, 650, 1100), "light", "scale=0.45"),
    ("one", SLOP, "slop_splat_3", (660, 1000, 830, 1100), "light", "scale=0.45"),
    ("one", SLOP, "slop_splat_4", (845, 1000, 995, 1100), "light", "scale=0.45"),
    ("one", SLOP, "slop_splat_5", (1015, 1000, 1160, 1100), "light", "scale=0.45"),
    ("one", SLOP, "slop_piece_0", (160, 1110, 275, 1195), "light", "scale=0.28"),
    ("one", SLOP, "slop_piece_1", (335, 1110, 455, 1195), "light", "scale=0.28"),
    ("one", SLOP, "slop_piece_2", (500, 1110, 630, 1195), "light", "scale=0.28"),
    ("one", SLOP, "slop_piece_3", (675, 1110, 800, 1195), "light", "scale=0.28"),
    ("one", SLOP, "slop_piece_4", (845, 1110, 985, 1195), "light", "scale=0.28"),
    ("one", SLOP, "slop_piece_5", (1025, 1110, 1150, 1195), "light", "scale=0.28"),
    # Fabio turned Jedi knight (his sheet's first six rows, shrunk to his size), the green
    # lightsaber, the burst it sends and its sparks, and the old knight's portrait.
    ("band", FABIO_JEDI, (0, 30, 640, 225), ["fab_jedi_idle_0", "fab_jedi_idle_1", "fab_jedi_idle_2", "fab_jedi_idle_3", "fab_jedi_idle_4"], "light", "scale=0.85"),
    ("band", FABIO_JEDI, (0, 260, 1254, 420), ["fab_jedi_walk_0", "fab_jedi_walk_1", "fab_jedi_walk_2", "fab_jedi_walk_3", "fab_jedi_walk_4", "fab_jedi_walk_5", "fab_jedi_walk_6", "fab_jedi_walk_7"], "light", "scale=0.85"),
    ("band", FABIO_JEDI, (0, 460, 1254, 620), ["fab_jedi_run_0", "fab_jedi_run_1", "fab_jedi_run_2", "fab_jedi_run_3", "fab_jedi_run_4", "fab_jedi_run_5", "fab_jedi_run_6", "fab_jedi_run_7"], "light", "scale=0.85"),
    ("band", FABIO_JEDI, (0, 640, 1254, 830), ["fab_jedi_jump_0", "fab_jedi_jump_1", "fab_jedi_jump_2", "fab_jedi_jump_3", "fab_jedi_jump_4"], "light", "scale=0.85"),
    ("band", FABIO_JEDI, (0, 850, 1254, 1040), ["fab_jedi_swing_0", "fab_jedi_swing_1", "fab_jedi_swing_2", "fab_jedi_swing_3", "fab_jedi_swing_4", "fab_jedi_swing_5"], "light", "scale=0.85"),
    ("band", FABIO_JEDI, (0, 1060, 390, 1220), ["fab_jedi_hurt_0", "fab_jedi_hurt_1"], "light", "scale=0.85"),
    ("band", JEDI_ITEMS, (60, 50, 1254, 240), ["saber_0", "saber_1", "saber_2", "saber_3"], "light", "scale=0.13"),
    ("band", JEDI_ITEMS, (40, 300, 1254, 450), ["force_burst_0", "force_burst_1", "force_burst_2", "force_burst_3", "force_burst_4", "force_burst_5"], "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "force_hit_0", (27, 500, 217, 670), "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "force_hit_1", (225, 500, 415, 670), "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "force_hit_2", (433, 500, 623, 670), "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "force_hit_3", (640, 500, 830, 670), "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "force_hit_4", (835, 500, 1025, 670), "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "force_hit_5", (1043, 500, 1233, 670), "light", "scale=0.5"),
    ("one", JEDI_ITEMS, "old_knight_0", (350, 730, 544, 925), "light", "scale=0.6"),
    ("one", JEDI_ITEMS, "old_knight_1", (710, 730, 904, 925), "light", "scale=0.6"),
    # Scenery zones: ground and fill tiles for snow, sand and cave (four variants each, cut to the
    # size of a tile), and the decorations that stand in each zone.
    ("one", BIOME_GROUND, "ground_snow_0", (324, 6, 522, 202), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_snow_1", (544, 6, 740, 202), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_snow_2", (762, 6, 960, 202), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_snow_3", (982, 6, 1182, 202), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_snow_0", (328, 214, 518, 392), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_snow_1", (546, 214, 738, 392), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_snow_2", (766, 214, 958, 392), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_snow_3", (986, 214, 1180, 392), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_sand_0", (326, 402, 520, 586), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_sand_1", (544, 402, 740, 586), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_sand_2", (762, 402, 960, 586), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_sand_3", (984, 400, 1182, 586), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_sand_0", (328, 600, 518, 766), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_sand_1", (546, 600, 738, 766), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_sand_2", (766, 600, 956, 766), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_sand_3", (986, 600, 1178, 766), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_cave_0", (324, 768, 520, 960), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_cave_1", (544, 780, 740, 960), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_cave_2", (764, 770, 960, 960), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "ground_cave_3", (984, 776, 1182, 960), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_cave_0", (328, 974, 516, 1154), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_cave_1", (548, 974, 738, 1154), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_cave_2", (766, 974, 958, 1154), "light", "scale=0.168"),
    ("one", BIOME_GROUND, "fill_cave_3", (986, 974, 1180, 1154), "light", "scale=0.168"),
    ("one", BIOME_DECOR, "decor_snow_pine_0", (40, 55, 266, 332), "light", "scale=0.42"),
    ("one", BIOME_DECOR, "decor_snow_pine_1", (290, 118, 464, 332), "light", "scale=0.42"),
    ("one", BIOME_DECOR, "decor_snow_pine_2", (482, 175, 614, 332), "light", "scale=0.42"),
    ("one", BIOME_DECOR, "decor_snowman", (626, 95, 838, 332), "light", "scale=0.3"),
    ("one", BIOME_DECOR, "decor_ice", (840, 137, 1024, 334), "light", "scale=0.3"),
    ("one", BIOME_DECOR, "decor_frost_bush", (1030, 167, 1236, 332), "light", "scale=0.3"),
    ("one", BIOME_DECOR, "decor_cactus_0", (46, 396, 232, 624), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_cactus_1", (242, 426, 416, 624), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_cactus_2", (424, 464, 578, 622), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_dead_tree", (590, 406, 810, 624), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_skull", (780, 494, 1028, 624), "light", "scale=0.35"),
    ("one", BIOME_DECOR, "decor_boulder", (1032, 458, 1238, 624), "light", "scale=0.35"),
    ("one", BIOME_DECOR, "decor_crystal_0", (46, 697, 232, 875), "light", "scale=0.34"),
    ("one", BIOME_DECOR, "decor_crystal_1", (252, 707, 420, 875), "light", "scale=0.34"),
    ("one", BIOME_DECOR, "decor_crystal_2", (442, 675, 614, 875), "light", "scale=0.34"),
    ("one", BIOME_DECOR, "decor_mushroom", (632, 713, 822, 875), "light", "scale=0.34"),
    ("one", BIOME_DECOR, "decor_stalagmite_0", (846, 683, 1018, 875), "light", "scale=0.34"),
    ("one", BIOME_DECOR, "decor_stalagmite_1", (1044, 743, 1222, 875), "light", "scale=0.34"),
    ("one", BIOME_DECOR, "decor_stalactite_0", (96, 956, 296, 1162), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_stalactite_1", (358, 956, 534, 1130), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_stalactite_2", (590, 956, 776, 1124), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_stalactite_3", (834, 956, 980, 1092), "light", "scale=0.4"),
    ("one", BIOME_DECOR, "decor_stalactite_4", (1038, 956, 1184, 1066), "light", "scale=0.4"),
    # Fabio's three friends: Andrea, Alessandro and Giulia (idle, walk, jump, attack and the effect
    # of the attack), the three pickups that bring them and the poof they arrive in.
    ("band", ALLY_ANDREA, (150, 40, 1254, 240), ["ally_andrea_idle_0", "ally_andrea_idle_1", "ally_andrea_idle_2", "ally_andrea_idle_3"], "light", "scale=0.84"),
    ("band", ALLY_ANDREA, (150, 270, 1254, 460), ["ally_andrea_walk_0", "ally_andrea_walk_1", "ally_andrea_walk_2", "ally_andrea_walk_3", "ally_andrea_walk_4", "ally_andrea_walk_5"], "light", "scale=0.84"),
    ("band", ALLY_ANDREA, (150, 490, 1254, 690), ["ally_andrea_jump_0", "ally_andrea_jump_1", "ally_andrea_jump_2"], "light", "scale=0.84"),
    ("one", ALLY_ANDREA, "ally_andrea_attack_0", (175, 752, 320, 920), "light", "scale=0.84"),
    ("one", ALLY_ANDREA, "ally_andrea_attack_1", (330, 735, 500, 920), "light", "scale=0.84"),
    ("one", ALLY_ANDREA, "ally_andrea_attack_2", (500, 735, 700, 920), "light", "scale=0.84"),
    ("one", ALLY_ANDREA, "ally_andrea_attack_3", (690, 735, 880, 920), "light", "scale=0.84"),
    ("one", ALLY_ANDREA, "ally_andrea_attack_4", (880, 735, 1050, 920), "light", "scale=0.84"),
    ("one", ALLY_ANDREA, "ally_andrea_attack_5", (1050, 735, 1230, 920), "light", "scale=0.84"),
    ("band", ALLY_ALESSANDRO, (150, 40, 1254, 240), ["ally_alessandro_idle_0", "ally_alessandro_idle_1", "ally_alessandro_idle_2", "ally_alessandro_idle_3"], "light", "scale=0.88"),
    ("band", ALLY_ALESSANDRO, (150, 270, 1254, 470), ["ally_alessandro_walk_0", "ally_alessandro_walk_1", "ally_alessandro_walk_2", "ally_alessandro_walk_3", "ally_alessandro_walk_4", "ally_alessandro_walk_5"], "light", "scale=0.88"),
    ("band", ALLY_ALESSANDRO, (150, 510, 1254, 700), ["ally_alessandro_jump_0", "ally_alessandro_jump_1", "ally_alessandro_jump_2"], "light", "scale=0.88"),
    ("band", ALLY_ALESSANDRO, (70, 740, 1254, 940), ["ally_alessandro_attack_0", "ally_alessandro_attack_1", "ally_alessandro_attack_2", "ally_alessandro_attack_3", "ally_alessandro_attack_4", "ally_alessandro_attack_5"], "light", "scale=0.88"),
    ("band", ALLY_ALESSANDRO, (140, 980, 1254, 1130), ["ally_alessandro_fx_0", "ally_alessandro_fx_1", "ally_alessandro_fx_2", "ally_alessandro_fx_3"], "light", "scale=0.8"),
    ("band", ALLY_GIULIA, (150, 50, 1254, 260), ["ally_giulia_idle_0", "ally_giulia_idle_1", "ally_giulia_idle_2", "ally_giulia_idle_3"], "light", "scale=0.74"),
    ("band", ALLY_GIULIA, (150, 290, 1254, 500), ["ally_giulia_walk_0", "ally_giulia_walk_1", "ally_giulia_walk_2", "ally_giulia_walk_3", "ally_giulia_walk_4", "ally_giulia_walk_5"], "light", "scale=0.74"),
    ("band", ALLY_GIULIA, (150, 530, 1254, 740), ["ally_giulia_jump_0", "ally_giulia_jump_1", "ally_giulia_jump_2"], "light", "scale=0.74"),
    ("band", ALLY_GIULIA, (70, 780, 1254, 980), ["ally_giulia_attack_0", "ally_giulia_attack_1", "ally_giulia_attack_2", "ally_giulia_attack_3", "ally_giulia_attack_4", "ally_giulia_attack_5"], "light", "scale=0.74"),
    ("one", ALLY_ANDREA, "ally_andrea_fx_0", (95, 945, 285, 1165), "light", "scale=0.72"),
    ("one", ALLY_ANDREA, "ally_andrea_fx_1", (325, 945, 595, 1165), "light", "scale=0.72"),
    ("one", ALLY_ANDREA, "ally_andrea_fx_2", (625, 945, 895, 1165), "light", "scale=0.72"),
    ("one", ALLY_ANDREA, "ally_andrea_fx_3", (945, 945, 1195, 1165), "light", "scale=0.72"),
    ("one", ALLY_GIULIA, "ally_giulia_fx_0", (190, 1010, 320, 1145), "light", "scale=0.5"),
    ("one", ALLY_GIULIA, "ally_giulia_fx_1", (380, 1010, 540, 1145), "light", "scale=0.5"),
    ("one", ALLY_GIULIA, "ally_giulia_fx_2", (600, 1010, 820, 1145), "light", "scale=0.5"),
    ("one", ALLY_GIULIA, "ally_giulia_fx_3", (890, 1010, 1020, 1145), "light", "scale=0.5"),
    # Giacomo: a swimmer who throws a water polo ball; sitting with a black eye after Giulia has punched him.
    ("band", ALLY_GIACOMO, (150, 40, 1254, 255), ["ally_giacomo_idle_0", "ally_giacomo_idle_1", "ally_giacomo_idle_2", "ally_giacomo_idle_3"], "light", "scale=0.74"),
    ("band", ALLY_GIACOMO, (150, 280, 1254, 495), ["ally_giacomo_walk_0", "ally_giacomo_walk_1", "ally_giacomo_walk_2", "ally_giacomo_walk_3", "ally_giacomo_walk_4", "ally_giacomo_walk_5"], "light", "scale=0.74"),
    ("band", ALLY_GIACOMO, (150, 515, 1254, 705), ["ally_giacomo_jump_0", "ally_giacomo_jump_1", "ally_giacomo_jump_2"], "light", "scale=0.74"),
    ("band", ALLY_GIACOMO, (150, 715, 1254, 895), ["ally_giacomo_attack_0", "ally_giacomo_attack_1", "ally_giacomo_attack_2", "ally_giacomo_attack_3", "ally_giacomo_attack_4", "ally_giacomo_attack_5"], "light", "scale=0.74"),
    ("one", ALLY_GIACOMO, "ally_giacomo_sit_0", (265, 890, 445, 1060), "light", "scale=0.74"),
    ("one", ALLY_GIACOMO, "ally_giacomo_sit_1", (505, 890, 690, 1060), "light", "scale=0.74"),
    # Giacomo crouched, rolling the ball at something crawling on the floor.
    ("one", GIACOMO_LOW, "ally_giacomo_low_0", (50, 590, 190, 810), "light", "scale=0.74"),
    ("one", GIACOMO_LOW, "ally_giacomo_low_1", (225, 630, 400, 810), "light", "scale=0.74"),
    ("one", GIACOMO_LOW, "ally_giacomo_low_2", (455, 630, 610, 810), "light", "scale=0.74"),
    ("one", GIACOMO_LOW, "ally_giacomo_low_3", (650, 630, 840, 810), "light", "scale=0.74"),
    ("one", GIACOMO_LOW, "ally_giacomo_low_4", (870, 630, 1030, 810), "light", "scale=0.74"),
    ("one", GIACOMO_LOW, "ally_giacomo_low_5", (1080, 580, 1210, 810), "light", "scale=0.74"),
    ("band", ALLY_GIACOMO, (200, 1070, 1000, 1170), ["ally_giacomo_fx_0", "ally_giacomo_fx_1", "ally_giacomo_fx_2", "ally_giacomo_fx_3"], "light", "scale=0.32"),
    ("band", ALLY_GIACOMO, (200, 1070, 1000, 1170), ["item_polo_ball_0", "item_polo_ball_1", "item_polo_ball_2", "item_polo_ball_3"], "light", "scale=0.32"),
    ("one", ALLY_ITEMS, "item_sushi_0", (88, 80, 288, 280), "light", "scale=0.22"),
    ("one", ALLY_ITEMS, "item_sushi_1", (385, 80, 585, 280), "light", "scale=0.22"),
    ("one", ALLY_ITEMS, "item_sushi_2", (671, 80, 871, 280), "light", "scale=0.22"),
    ("one", ALLY_ITEMS, "item_sushi_3", (975, 80, 1175, 280), "light", "scale=0.22"),
    ("one", ALLY_ITEMS, "item_dragon_0", (80, 370, 280, 570), "light", "scale=0.17"),
    ("one", ALLY_ITEMS, "item_dragon_1", (380, 370, 580, 570), "light", "scale=0.17"),
    ("one", ALLY_ITEMS, "item_dragon_2", (675, 370, 875, 570), "light", "scale=0.17"),
    ("one", ALLY_ITEMS, "item_dragon_3", (975, 370, 1175, 570), "light", "scale=0.17"),
    ("one", ALLY_ITEMS, "item_gloves_0", (80, 670, 280, 860), "light", "scale=0.18"),
    ("one", ALLY_ITEMS, "item_gloves_1", (378, 670, 578, 860), "light", "scale=0.18"),
    ("one", ALLY_ITEMS, "item_gloves_2", (672, 670, 872, 860), "light", "scale=0.18"),
    ("one", ALLY_ITEMS, "item_gloves_3", (970, 670, 1170, 860), "light", "scale=0.18"),
    ("one", ALLY_ITEMS, "ally_poof_0", (20, 910, 210, 1110), "light", "scale=0.55"),
    ("one", ALLY_ITEMS, "ally_poof_1", (210, 910, 400, 1110), "light", "scale=0.55"),
    ("one", ALLY_ITEMS, "ally_poof_2", (400, 910, 590, 1110), "light", "scale=0.55"),
    ("one", ALLY_ITEMS, "ally_poof_3", (623, 910, 813, 1110), "light", "scale=0.55"),
    ("one", ALLY_ITEMS, "ally_poof_4", (845, 910, 1035, 1110), "light", "scale=0.55"),
    ("one", ALLY_ITEMS, "ally_poof_5", (1050, 910, 1240, 1110), "light", "scale=0.55"),
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
            if "main" in entry[5:]:
                sprite = tight(keyed, max(blobs(keyed), key=len))
            else:
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
