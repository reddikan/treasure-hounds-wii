#!/usr/bin/env python3
"""
TREASURE HOUNDS - level builder.

Each level is drawn with a few helpers (rects, blobs, paths, scatter) instead of typed by hand,
then written to source/maps.c together with its triggers, puzzle groups, signs and hooks.
Run:  python3 tools/maps/make_maps.py      (also writes tools/out/map_N.png previews)
"""
import math, random, os, sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')

class Map:
    def __init__(s, name, sub, w, h, theme, fill='.'):
        s.name, s.sub, s.w, s.h, s.theme = name, sub, w, h, theme
        s.g = [[fill] * w for _ in range(h)]
        s.trig, s.group, s.sign, s.pipe, s.organ, s.hook, s.boul = [], [], [], [], [], [], []
        s.rng = random.Random(hash(name) & 0xFFFF)

    def ok(s, x, y): return 0 <= x < s.w and 0 <= y < s.h
    def get(s, x, y): return s.g[y][x] if s.ok(x, y) else '#'
    def put(s, x, y, c):
        if s.ok(x, y): s.g[y][x] = c
    def rect(s, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1): s.put(x, y, c)
    def box(s, x0, y0, x1, y1, c, top=2):
        """walls round a room: `top` rows thick at the top so the back wall shows a face"""
        s.rect(x0, y0, x1, y0 + top - 1, c)
        s.rect(x0, y1, x1, y1, c)
        s.rect(x0, y0, x0, y1, c)
        s.rect(x1, y0, x1, y1, c)
    def blob(s, cx, cy, rx, ry, c, rough=0.25, only=None):
        for y in range(int(cy - ry - 2), int(cy + ry + 3)):
            for x in range(int(cx - rx - 2), int(cx + rx + 3)):
                dx, dy = (x + .5 - cx) / rx, (y + .5 - cy) / ry
                n = 1 + rough * (math.sin(x * 1.7 + y * .9) * .5 + math.sin(x * .6 - y * 1.3) * .5)
                if dx * dx + dy * dy <= n and (only is None or s.get(x, y) in only):
                    s.put(x, y, c)
    def path(s, pts, c, wid=1, only=None):
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            n = max(abs(bx - ax), abs(by - ay)) * 3 + 1
            for i in range(n + 1):
                t = i / n
                x, y = ax + (bx - ax) * t, ay + (by - ay) * t
                for oy in range(-(wid // 2), wid - wid // 2):
                    for ox in range(-(wid // 2), wid - wid // 2):
                        px, py = int(round(x)) + ox, int(round(y)) + oy
                        if only is None or s.get(px, py) in only: s.put(px, py, c)
    def scatter(s, x0, y0, x1, y1, c, dens, on='.', clear=0):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if s.get(x, y) in on and s.rng.random() < dens:
                    if clear and any(s.get(x + i, y + j) not in on for i in range(-clear, clear + 1) for j in range(-clear, clear + 1)):
                        continue
                    s.put(x, y, c)
    def trigger(s, x0, y0, x1, y1, scene, flags='ONCE'):
        s.trig.append((x0, y0, x1, y1, scene, flags))
    def signp(s, x, y, ch, text):
        s.put(x, y, ch); s.sign.append((y, x, text))
    def gate_group(s, gx, gy, cond, x0=0, y0=0, x1=0, y1=0):
        s.group.append((gx, gy, cond, x0, y0, x1, y1))
    def gate_index(s, gx, gy):
        gates = [(y, x) for y in range(s.h) for x in range(s.w) if s.g[y][x] == 'D']
        assert (gy, gx) in gates, (s.name, 'no gate at', gx, gy, gates)
        return gates.index((gy, gx))

    def check(s):
        for y, row in enumerate(s.g):
            assert len(row) == s.w, (s.name, y)
        st = [(x, y) for y in range(s.h) for x in range(s.w) if s.g[y][x] == '/']
        assert len(st) == 1, (s.name, 'start', st)

    def rows(s): return [''.join(r) for r in s.g]


# ======================================================================================
# LEVEL 1 - GULL'S POINT: a stormy fishing town
# ======================================================================================
def level1():
    m = Map("Gull's Point", "Chapter 1", 80, 52, 'THEME_TOWN', '.')
    W, H = m.w, m.h
    # edges
    m.rect(0, 0, W - 1, 2, '#'); m.rect(0, 0, 1, H - 1, '#'); m.rect(0, H - 2, W - 1, H - 1, '#')
    # the sea along the east, with a ragged shore
    for y in range(3, H - 2):
        if y <= 14: shore = 68
        else: shore = 63 + int(2 * math.sin(y * .45)) + (1 if y % 7 == 0 else 0)
        for x in range(shore, W): m.put(x, y, '~')
        m.put(shore - 1, y, '`') if y > 16 else None
    m.rect(66, 3, 67, 14, '#')                        # plateau cliff above the sea
    m.rect(68, 15, 79, 16, '~')
    # the cliff band that separates the lighthouse plateau from the town (ramp at x 27..29)
    m.rect(2, 15, 65, 16, '#')
    m.rect(26, 15, 30, 16, ',')
    m.rect(64, 15, 65, 17, '#')

    # ---- town square ----
    m.rect(25, 21, 43, 33, ':')
    m.put(34, 27, 'V')
    for (x, y) in [(30, 24), (38, 24), (30, 31), (38, 31)]: m.put(x, y, 'n')
    for (x, y) in [(26, 22), (42, 22), (26, 32), (42, 32)]: m.put(x, y, 'L')
    m.signp(33, 33, 's', "GULL'S POINT\nFounded 1702 by sailors, smugglers\nand at least one pirate.")
    m.signp(35, 21, 's', "NOTICE: The Shoals will be torn down\nMONDAY for the new Gull's Point\nGolf & Yacht Club.  - Thornbury Development")
    for (x, y) in [(29, 27), (40, 28), (36, 22), (31, 33)]: m.put(x, y, 'q')
    # roads
    m.path([(4, 44), (22, 44), (28, 39), (31, 34)], ',', 3)
    m.path([(43, 28), (48, 28), (53, 29)], ',', 3)
    m.path([(28, 21), (28, 14)], ',', 3)
    m.path([(28, 13), (36, 12), (46, 11), (55, 11), (60, 12)], ',', 2)

    # ---- the Shoals (home) ----
    m.put(8, 41, 'S'); m.put(19, 41, 'S')
    m.put(8, 44, '/')
    for x in (4, 14, 24): m.put(x, 46, 'L')
    m.signp(13, 42, 's', "THE SHOALS\nHome sweet home.\n(For two more days.)")
    for x in range(3, 26): m.put(x, 48, 'f')
    m.put(15, 48, '.'); m.put(16, 48, '.')
    m.scatter(2, 36, 26, 40, 'T', 0.10, on='.', clear=0)
    m.rect(2, 36, 26, 36, '.')
    for (x, y) in [(3, 49), (6, 49), (12, 49), (20, 49), (25, 49)]: m.put(x, y, 'T')
    m.put(25, 43, 'B'); m.put(2, 43, 'C')
    m.put(17, 46, 'q'); m.put(10, 46, 'q')

    # ---- Widow Hatch's garden: hedge rings ----
    m.box(3, 19, 21, 35, 'h', top=2)
    m.rect(11, 35, 13, 35, '.')
    for x in (11, 12, 13): m.put(x, 35, 'b')
    m.box(7, 24, 17, 31, 'h', top=1)
    m.rect(15, 24, 16, 24, '.')
    m.put(12, 28, '2')                                # gear #2
    m.put(11, 27, 'i'); m.put(13, 29, 'c'); m.put(10, 29, 'c'); m.put(14, 26, 'r')
    m.put(19, 22, '3')                                # a doubloon
    for (x, y) in [(5, 22), (19, 33), (5, 33), (9, 22)]: m.put(x, y, 'c')
    m.put(6, 27, '8'); m.put(19, 28, '8')
    m.scatter(4, 21, 20, 34, 'b', 0.05, on='.', clear=1)
    m.signp(12, 37, 's', "PRIVATE GARDEN\nKEEP OUT\n- Widow Hatch")

    # ---- docks + the Crowleys' cannery ----
    m.rect(47, 23, 58, 33, ':')
    m.put(53, 27, 'S')
    # Tiny's yard behind the cannery
    for x in range(47, 59): m.put(x, 18, 'f')
    m.rect(47, 19, 58, 22, ',')
    m.put(53, 21, '{')
    m.put(48, 19, 'B'); m.put(57, 19, 'C'); m.put(57, 20, 'C')
    # boardwalk + piers
    m.rect(59, 19, 62, 46, '=')
    m.rect(63, 21, 78, 22, '=')
    m.rect(63, 39, 77, 40, '=')
    for (x, y) in [(66, 21), (69, 22), (64, 40), (70, 39)]: m.put(x, y, 'B')
    for (x, y) in [(72, 21), (72, 22), (73, 21), (73, 22)]: m.put(x, y, 'C')
    m.put(77, 21, '2')                                # gear #1, behind the crates
    m.put(76, 39, '3')                                # doubloon on the far pier
    m.put(74, 39, 'B'); m.put(74, 40, 'B')
    m.put(67, 43, 'R'); m.put(71, 36, 'R')
    for (x, y) in [(60, 24), (60, 33), (60, 42)]: m.put(x, y, 'L')
    for (x, y) in [(61, 28), (61, 37)]: m.put(x, y, 'e')
    m.put(62, 45, 'C'); m.put(59, 45, 'B'); m.put(62, 30, 'A')
    m.put(49, 31, 'C'); m.put(50, 31, 'C'); m.put(56, 31, 'B')
    m.put(48, 30, '<')                                # Vic patrols the cannery road
    m.put(60, 35, '>')                                # Lenny on the boardwalk
    m.put(50, 25, '$'); m.put(56, 25, '4')

    # shore between town and docks
    m.scatter(44, 34, 58, 48, 'o', 0.04, on='.')
    m.put(52, 42, 'O'); m.put(47, 46, 'O')
    m.put(55, 38, 'q'); m.put(49, 40, '4')

    # ---- the plateau: pines, graveyard, lighthouse ----
    m.scatter(2, 3, 32, 13, 'T', 0.16, on='.', clear=0)
    m.rect(24, 12, 32, 14, '.'); m.path([(28, 13), (36, 12), (46, 11), (55, 11), (60, 12)], ',', 2)
    m.rect(4, 4, 9, 7, ',')
    m.put(6, 5, 'F'); m.put(7, 7, '2')                # gear #3 at the old lookout camp
    m.put(4, 4, 'C'); m.put(9, 4, 'B'); m.put(5, 7, 'n')
    m.put(10, 9, '8'); m.put(16, 6, '8'); m.put(22, 11, '8')
    for x in range(34, 46):
        m.put(x, 4, 'f'); m.put(x, 10, 'f')
    for y in range(5, 10): m.put(34, y, 'f'); m.put(45, y, 'f')
    m.put(45, 7, ','); m.put(45, 8, ',')
    for (x, y) in [(36, 6), (39, 6), (42, 6), (36, 8), (39, 8), (42, 8)]: m.put(x, y, '+')
    m.signp(44, 6, 's', "Here lies OLD SALT PETE\nwho said 'that wave looks friendly'.")
    m.put(37, 9, '3')                                 # doubloon among the graves
    m.put(56, 10, 'Q')
    m.put(61, 12, 'J'); m.put(63, 12, 'G')
    m.put(52, 6, 'O'); m.put(60, 6, 'o'); m.put(63, 8, 'o'); m.put(49, 13, 'o')
    m.put(58, 13, 'L'); m.put(52, 13, 'H')
    m.scatter(46, 3, 64, 6, 'T', 0.12, on='.')
    m.signp(62, 9, 's', "DANGER: OLD SMUGGLERS' TUNNEL\nGrate sealed by order of the\nGull's Point Harbor Master")

    # stray trees and bushes in the lower town
    m.scatter(2, 17, 24, 18, 'T', 0.3, on='.')
    m.scatter(22, 19, 24, 35, 'T', 0.2, on='.')
    m.scatter(44, 17, 46, 33, 'T', 0.18, on='.')
    m.scatter(30, 34, 46, 49, 'b', 0.03, on='.', clear=1)
    m.scatter(30, 40, 46, 49, 'T', 0.06, on='.', clear=1)
    m.scatter(30, 17, 44, 20, 'b', 0.10, on='.')
    for (x, y) in [(33, 41), (40, 45)]: m.put(x, y, '$')
    m.put(38, 38, '4')

    # triggers
    m.trigger(25, 20, 43, 34, 'SC_L1_SQUARE', 'ONCE|CHECKPOINT')
    m.trigger(9, 36, 15, 38, 'SC_L1_GARDEN')
    m.trigger(44, 23, 46, 33, 'SC_L1_CROWLEYS', 'ONCE|CHECKPOINT')
    m.trigger(47, 19, 58, 22, 'SC_L1_TINY')
    m.trigger(25, 11, 31, 13, 'SC_L1_PLATEAU', 'ONCE|CHECKPOINT')
    m.trigger(59, 41, 62, 46, 'SC_L1_DOCKS')
    m.trigger(10, 21, 20, 23, 'SC_L1_INGARDEN')
    m.trigger(63, 12, 63, 12, 'SC_EXIT', 'EXIT')
    m.check()
    return m


# ======================================================================================
# LEVEL 2 - SMUGGLERS' CAVES: pitch dark, only the lantern
# ======================================================================================
def level2():
    m = Map("Smugglers' Caves", "Chapter 2", 88, 58, 'THEME_CAVE', '#')
    def room(cx, cy, rx, ry, rough=.28): m.blob(cx, cy, rx, ry, '.', rough)
    def tunnel(pts, w=3): m.path(pts, '.', w)
    # 1. landing chamber under the grate
    room(8, 7, 6, 4.5)
    m.put(7, 5, '/')
    m.put(4, 8, 'u'); m.put(11, 9, 'u'); m.put(12, 5, 'a'); m.put(3, 6, 'a')
    m.put(10, 9, 'C')
    # 2. the wishing well chamber (coins rain down the old town well shaft)
    tunnel([(13, 7), (24, 8)], 3)
    room(27, 8, 6, 4.5)
    for (x, y) in [(26, 7), (28, 8), (27, 10), (25, 9), (29, 6)]: m.put(x, y, '$')
    m.put(31, 6, 'z'); m.put(23, 6, 'u'); m.put(30, 11, 'a')
    m.put(24, 10, '6')
    # 3. down to the underground river
    tunnel([(27, 12), (27, 20)], 3)
    room(27, 23, 9, 4.5)
    m.rect(17, 25, 38, 27, '~')                       # the river
    m.rect(26, 25, 28, 27, '=')                       # plank bridge
    m.blob(27, 31, 8, 3.5, '.')
    m.put(20, 23, '7'); m.put(33, 30, '7'); m.put(22, 31, '7')
    m.put(19, 21, 'z'); m.put(35, 22, 'u'); m.put(21, 30, 'u'); m.put(34, 32, 'a')
    m.put(31, 32, '3')                                # doubloon by the river
    # 4. spike corridor east
    tunnel([(35, 31), (47, 31)], 3)
    for x in range(38, 46):
        if x % 2 == 0:
            for y in (30, 31, 32): m.put(x, y, '^')
    # 5. brazier hall
    m.rect(48, 23, 62, 36, '.')
    m.box(47, 21, 63, 37, '#', top=2)
    m.rect(47, 30, 47, 32, '.')                       # entrance from the west
    for (x, y) in [(50, 25), (60, 25), (50, 34), (60, 34)]: m.put(x, y, '&')
    m.signp(55, 29, 'E', "FOUR FIRES TO WAKE THE DOOR")
    for x in (49, 61): m.put(x, 23, 't')
    m.put(55, 22, 'D')                                # gate 0, north wall
    m.put(56, 22, '.')
    m.gate_group(55, 22, 'COND_BRAZIERS', 48, 23, 62, 36)
    m.put(52, 31, '6'); m.put(58, 27, '6')
    # 6. the crumbling bridge over the chasm (north of the hall)
    tunnel([(55, 21), (55, 17)], 2)
    m.rect(44, 9, 66, 16, ' ')
    m.rect(54, 9, 56, 16, '_')
    m.rect(52, 5, 58, 8, '.'); m.rect(52, 17, 58, 17, '.')
    m.blob(55, 5, 6, 3, '.')
    # 7. the four plates (teamwork)
    tunnel([(61, 5), (70, 5)], 3)
    m.rect(68, 3, 82, 14, '.')
    m.box(67, 1, 83, 15, '#', top=2)
    m.rect(67, 4, 67, 6, '.')
    for (x, y) in [(71, 6), (79, 6), (71, 11), (79, 11)]: m.put(x, y, 'p')
    m.signp(75, 8, 'E', "NO HOUND CROSSES ALONE")
    m.gate_group(74, 15, 'COND_PLATES', 68, 3, 82, 14)
    m.put(69, 13, 'C'); m.put(81, 13, 'B'); m.put(81, 4, 'u'); m.put(69, 4, 'z')
    # 8. Lenny's smuggler den
    tunnel([(74, 16), (74, 22)], 2)
    m.put(74, 15, 'D'); m.put(75, 15, '.'); m.put(73, 15, '#')   # the gate in the plates room's south wall
    room(74, 27, 9, 5)
    m.put(70, 26, 'N'); m.put(77, 25, 'F'); m.put(80, 29, 'B'); m.put(80, 28, 'B'); m.put(68, 29, 'C'); m.put(69, 30, 'C')
    m.put(74, 29, '>')
    m.put(78, 30, '4'); m.put(67, 25, '$'); m.put(81, 26, '$')
    m.put(70, 31, 'H')
    # 9. the chasm with the swing hooks
    tunnel([(74, 32), (74, 37)], 3)
    m.rect(66, 38, 84, 41, '.')
    m.rect(64, 42, 86, 45, ' ')
    m.rect(66, 46, 84, 48, '.')
    m.put(74, 41, '@'); m.put(74, 46, '@')
    m.hook.append((74, 41, 74, 47)); m.hook.append((74, 46, 74, 40))
    m.put(70, 39, 'a'); m.put(79, 39, 'z'); m.put(82, 47, 'u')
    # 10. dart corridor west
    m.rect(40, 47, 66, 49, '.')
    for x in range(42, 64, 4): m.put(x, 46, 'd')
    for x in range(44, 64, 4): m.put(x, 50, 'd')
    # 11. boulder run south-west to the temple stairs
    m.rect(36, 44, 39, 55, '.')
    m.rect(8, 52, 39, 55, '.')
    m.put(37, 44, '(')
    m.boul.append((37, 44, 0, 1, 36, 50, 39, 51))     # rolls down when you pass row 50
    m.rect(4, 50, 9, 56, '.')
    m.put(5, 54, ')')
    m.put(20, 53, '3'); m.put(12, 51, 'a'); m.put(30, 55, 'u')
    m.put(60, 48, '$'); m.put(50, 48, '$'); m.put(46, 48, '4')
    # doubloon #3 in a hidden pocket off the river
    tunnel([(17, 22), (9, 18)], 2)
    room(7, 17, 3, 2.5)
    m.put(7, 17, '3'); m.put(5, 16, 'z')
    m.put(15, 20, '6')
    m.check()
    # triggers
    m.trigger(4, 4, 12, 10, 'SC_L2_INTRO', 'ONCE|CHECKPOINT')
    m.trigger(22, 5, 32, 11, 'SC_L2_WELL')
    m.trigger(19, 20, 35, 24, 'SC_L2_RIVER', 'ONCE|CHECKPOINT')
    m.trigger(36, 30, 37, 32, 'SC_L2_SPIKES')
    m.trigger(48, 29, 51, 33, 'SC_L2_BRAZIERS', 'ONCE|CHECKPOINT')
    m.trigger(53, 17, 57, 19, 'SC_L2_CHASM', 'ONCE|CHECKPOINT')
    m.trigger(68, 3, 70, 7, 'SC_L2_PLATES', 'ONCE|CHECKPOINT')
    m.trigger(70, 22, 78, 23, 'SC_L2_LENNY', 'ONCE|CHECKPOINT')
    m.trigger(66, 38, 84, 40, 'SC_L2_HOOKS', 'ONCE|CHECKPOINT')
    m.trigger(60, 47, 66, 49, 'SC_L2_DARTS', 'ONCE|CHECKPOINT')
    m.trigger(36, 46, 39, 48, 'SC_L2_BOULDER_PRE', 'ONCE|CHECKPOINT')
    m.trigger(4, 53, 7, 56, 'SC_EXIT', 'EXIT')
    return m


# ======================================================================================
# LEVEL 3 - TEMPLE OF BONES
# ======================================================================================
def level3():
    m = Map("Temple of Bones", "Chapter 3", 84, 60, 'THEME_TEMPLE', '#')
    # 1. entrance hall (south)
    m.rect(34, 44, 50, 56, '.')
    m.box(33, 42, 51, 57, '#', top=2)
    m.put(42, 54, '/')
    for (x, y) in [(36, 47), (48, 47), (36, 52), (48, 52)]: m.put(x, y, 'I')
    m.put(42, 46, 'v') if False else None
    m.signp(42, 46, 'E', "IRONJAW JACK'S TOMB\nTurn back, landlubbers. The gold is mine.\nAlso: wipe your feet.")
    for x in (35, 41, 43, 49): m.put(x, 43, 't')
    m.put(38, 49, 'X'); m.put(46, 55, 'x')
    m.rect(40, 42, 44, 43, '.')                        # north exit of the hall
    # 2. central water court
    m.rect(28, 26, 56, 41, '.')
    m.box(27, 24, 57, 42, '#', top=2)
    m.rect(40, 42, 44, 42, '.')
    m.rect(31, 29, 53, 38, '~')
    m.rect(40, 29, 44, 38, '.')                        # causeway north-south
    m.rect(31, 33, 53, 34, '.')                        # causeway east-west
    for (x, y) in [(36, 31), (48, 31), (36, 36), (48, 36)]: m.put(x, y, 'i')
    m.put(42, 33, 'Q') if False else None
    for x in (30, 38, 46, 54): m.put(x, 25, 't')
    m.put(29, 40, '9'); m.put(55, 27, '9')
    m.put(42, 31, 'g')
    m.rect(27, 33, 27, 34, '.'); m.rect(57, 33, 57, 34, '.')   # west + east doors
    m.rect(40, 24, 44, 25, '.')                        # north door (gated)
    m.put(41, 25, 'D'); m.rect(40, 24, 40, 25, '#'); m.rect(43, 24, 44, 25, '#')   # gate 1 -> organ
    # 3. west wing: push-block puzzle
    m.rect(8, 26, 26, 41, '.')
    m.box(7, 24, 27, 42, '#', top=2)
    m.rect(27, 33, 27, 34, '.')
    m.rect(8, 26, 26, 41, ',')
    m.rect(10, 28, 24, 39, '.')
    for (x, y) in [(13, 30), (21, 37), (17, 33)]: m.put(x, y, '[')
    for (x, y) in [(12, 36), (22, 30)]: m.put(x, y, 'p')
    m.put(17, 25, 'D'); m.put(18, 25, '.'); m.rect(16, 24, 16, 25, '#'); m.rect(19, 24, 19, 25, '#'); m.rect(17, 24, 18, 24, '.')
    m.gate_group(41, 25, 'COND_PLATES', 8, 26, 26, 41)         # the blocks open the court's north door
    m.gate_group(17, 25, 'COND_LEVER', 58, 26, 78, 41)          # the fire room's lever opens the treasure nook
    m.put(10, 27, 'i'); m.put(25, 40, 'I'); m.put(9, 40, 'X'); m.put(25, 27, 'c'); m.put(24, 27, 'c')
    for x in (12, 22): m.put(x, 25, 't')
    # the treasure nook behind the block gate
    m.rect(14, 17, 21, 23, '.')
    m.box(13, 15, 22, 24, '#', top=2)
    m.rect(17, 24, 18, 24, '.')
    m.put(17, 19, 'H'); m.put(15, 18, '3'); m.put(20, 21, 'g'); m.put(14, 22, '4')
    m.put(15, 16, 't'); m.put(20, 16, 't')
    # 4. east wing: flame jets + skeletons
    m.rect(58, 26, 78, 41, '.')
    m.box(57, 24, 79, 42, '#', top=2)
    m.rect(57, 33, 57, 34, '.')
    for y in (28, 30, 32, 35, 37, 39):
        for x in range(61, 77, 3):
            if (x + y) % 2 == 0: m.put(x, y, ';')
    m.put(70, 33, '9'); m.put(64, 38, '9'); m.put(74, 28, '9')
    m.put(77, 40, '3'); m.put(77, 27, '4'); m.put(77, 33, '!'); m.put(60, 27, 'c'); m.put(60, 40, 'c')
    for x in (60, 68, 76): m.put(x, 25, 't')
    # 5. the bone organ (north of the court)
    m.rect(32, 10, 52, 22, '.')
    m.box(31, 8, 53, 23, '#', top=2)
    m.rect(41, 23, 42, 23, '.')
    m.signp(42, 10, 'E', "PLAY THE SONG OF THE BONES")   # the tune carving
    notes = [4, 1, 3, 0, 2]                              # heights of the pipes left -> right
    for i, nt in enumerate(notes):
        m.put(35 + i * 4, 15, '|'); m.pipe.append(nt)
    m.organ = [2, 0, 3, 1, 4]                            # the tune: carving order (by pipe height)
    for x in (33, 37, 47, 51): m.put(x, 9, 'd')          # darters punish wrong notes
    for x in (34, 50): m.put(x, 9, 't') if False else None
    m.put(33, 21, 'X'); m.put(51, 20, 'x'); m.put(33, 14, '3'); m.put(51, 12, 'I'); m.put(33, 12, 'I')
    m.rect(40, 8, 40, 9, '#'); m.rect(43, 8, 43, 9, '#'); m.rect(41, 9, 42, 9, '.')
    # 6. the skull vault (crumbling path, then the key)
    m.rect(36, 2, 48, 7, '.')
    m.box(30, 0, 54, 8, '#', top=1)
    m.rect(41, 7, 42, 8, '.')
    m.rect(31, 1, 53, 7, '~')
    m.rect(40, 3, 43, 7, '.')
    m.rect(36, 3, 47, 4, '.')
    m.put(42, 3, 'Z')
    m.put(41, 5, '5')                                    # THE SKULL KEY
    for (x, y) in [(37, 3), (46, 3)]: m.put(x, y, 'g')
    m.put(39, 4, '&') if False else None
    m.put(41, 8, 'D'); m.put(42, 8, '.')
    m.gate_group(41, 8, 'COND_ORGAN')
    # escape: east exit from the court, opened by taking the key
    m.rect(78, 33, 79, 34, '.')
    m.put(79, 33, '#'); m.put(79, 34, '#')
    m.rect(79, 31, 83, 36, '#')
    m.rect(78, 44, 82, 46, '.')
    m.rect(76, 41, 77, 46, '.')
    m.put(76, 42, 'D'); m.put(77, 42, '.')                # opens when the key is taken
    m.gate_group(76, 42, 'COND_KEY')
    m.rect(75, 42, 75, 42, '#'); m.rect(78, 42, 79, 42, '#')
    m.rect(76, 43, 83, 46, '.')
    m.box(75, 42, 84, 47, '#', top=1) if False else None
    m.rect(82, 43, 83, 46, '.')
    m.put(81, 45, ')')
    m.check()
    m.trigger(34, 50, 50, 56, 'SC_L3_INTRO', 'ONCE|CHECKPOINT')
    m.trigger(38, 40, 46, 41, 'SC_L3_COURT', 'ONCE|CHECKPOINT')
    m.trigger(23, 32, 26, 35, 'SC_L3_BLOCKS', 'ONCE|CHECKPOINT')
    m.trigger(58, 32, 61, 35, 'SC_L3_FLAMES', 'ONCE|CHECKPOINT')
    m.trigger(38, 18, 46, 22, 'SC_L3_ORGAN', 'ONCE|CHECKPOINT')
    m.trigger(40, 6, 43, 7, 'SC_L3_VAULT', 'ONCE|CHECKPOINT')
    m.trigger(76, 43, 77, 44, 'SC_L3_ESCAPE', 'ONCE|CHECKPOINT')
    m.trigger(80, 43, 83, 46, 'SC_EXIT', 'EXIT')
    return m


# ======================================================================================
# LEVEL 4 - SKELETON COVE: the storm, the beach, and the pirate ship
# ======================================================================================
def level4():
    m = Map("Skeleton Cove", "Chapter 4", 90, 54, 'THEME_COVE', '.')
    W, H = m.w, m.h
    m.rect(0, 0, W - 1, 3, '#'); m.rect(0, 0, 1, H - 1, '#')
    # sea in the south and east, beach ragged
    for x in range(W):
        shore = 40 + int(2.5 * math.sin(x * .21)) + int(math.sin(x * .7))
        if x > 52: shore = min(shore, 36)
        for y in range(shore, H): m.put(x, y, '~')
        for y in range(shore - 3, shore): m.put(x, y, ',') if m.get(x, y) == '.' else None
        m.put(x, shore - 1, 'w') if x % 5 != 0 else None
    # the lagoon where the ship lies
    m.blob(70, 20, 17, 12, '~', 0.18, only='.,w')
    m.rect(84, 0, 89, H - 1, '~'); m.rect(84, 0, 89, 3, '#')
    # cliffs and the temple cave mouth (west)
    m.rect(2, 4, 9, 18, '#'); m.rect(10, 4, 20, 8, '#')
    m.rect(4, 19, 7, 21, '.')
    m.put(5, 20, '/')
    m.rect(3, 19, 3, 23, '#')
    # jungle edge along the top
    m.scatter(10, 9, 50, 13, 'P', 0.10, on='.')
    m.scatter(21, 4, 55, 8, 'P', 0.18, on='.')
    m.scatter(12, 9, 54, 14, 'r', 0.05, on='.')
    m.scatter(2, 22, 52, 34, 'P', 0.025, on='.', clear=1)
    # the shipwreck on the beach (like the old picture)
    m.put(22, 36, 'K'); m.put(28, 33, 'k'); m.put(17, 34, 'A'); m.put(14, 30, 'R')
    m.put(19, 31, 'B'); m.put(20, 32, 'C'); m.put(18, 31, 'C'); m.put(25, 30, 'B')
    for (x, y) in [(12, 36), (30, 38), (36, 37), (8, 33), (44, 38)]: m.put(x, y, '*')
    m.scatter(8, 26, 50, 39, 'o', 0.02, on='.,')
    m.put(11, 27, '7'); m.put(26, 37, '7'); m.put(40, 36, '7'); m.put(33, 34, '7')
    m.put(23, 27, '$'); m.put(31, 29, '$'); m.put(10, 32, '4'); m.put(13, 37, '3')
    # the Crowleys' camp
    m.rect(34, 17, 46, 25, ',')
    m.put(40, 21, 'F'); m.put(37, 19, 'n'); m.put(43, 23, 'n'); m.put(36, 23, 'B'); m.put(44, 18, 'C')
    m.put(38, 24, '<'); m.put(45, 21, '>')
    m.put(35, 18, 'L') if False else None
    # skeleton sentries and graves near the cliff
    for (x, y) in [(14, 12), (17, 14), (12, 15)]: m.put(x, y, '+')
    m.put(15, 13, '9'); m.put(27, 16, '9'); m.put(31, 25, '9')
    m.put(13, 10, '3')
    m.put(24, 20, 'y'); m.put(33, 12, 'y'); m.put(48, 15, 'y'); m.put(46, 15, '3')
    m.signp(50, 26, 's', "THE BLACK GULL\nCaptain: Ironjaw Jack\nCrew: very, very dead")
    # rocks out to the hook and the ship
    m.rect(50, 18, 54, 22, '.')
    m.put(53, 20, '@'); m.hook.append((53, 20, 58, 20))
    m.put(57, 20, '@'); m.hook.append((57, 20, 52, 20))
    # the ship: hull walls round a deck
    x0, x1, y0, y1 = 58, 82, 13, 27
    m.rect(x0, y0, x1, y1, 'U')
    m.rect(x0 + 1, y0 + 2, x1 - 1, y1 - 1, '-')
    m.rect(x0, 20, x0, 20, '-')                         # boarding gap where you land
    m.rect(x0, 19, x0, 21, '-')
    for (x, y) in [(64, 17), (72, 17)]: m.put(x, y, 'M')
    for (x, y) in [(61, 16), (61, 25), (69, 25), (75, 16)]: m.put(x, y, 'v')
    m.put(66, 24, 'B'); m.put(67, 24, 'B'); m.put(70, 15, 'C'); m.put(62, 21, 'e')
    for (x, y) in [(60, 15), (68, 15), (60, 26), (68, 26)]: m.put(x, y, 'l')
    m.put(76, 21, 'm')
    # the captain's cabin at the stern
    m.rect(77, 15, 81, 26, '"')
    m.rect(77, 15, 77, 19, 'W'); m.rect(77, 23, 77, 26, 'W')
    m.put(79, 20, 'Y')                                  # THE TREASURE CHEST
    m.put(80, 16, 'g'); m.put(80, 25, 'g'); m.put(78, 25, 'X'); m.put(81, 22, 'l')
    m.put(70, 21, '}')                                  # Ma Crowley waits on deck
    m.put(77, 20, 'D') if False else None
    m.check()
    m.trigger(3, 18, 9, 22, 'SC_L4_INTRO', 'ONCE|CHECKPOINT')
    m.trigger(14, 28, 26, 38, 'SC_L4_WRECK')
    m.trigger(30, 15, 33, 26, 'SC_L4_CAMP', 'ONCE|CHECKPOINT')
    m.trigger(48, 17, 55, 23, 'SC_L4_SHIP', 'ONCE|CHECKPOINT')
    m.trigger(58, 17, 63, 23, 'SC_L4_BOARD', 'ONCE')
    return m


# ======================================================================================
FLAGS = {'ONCE': 'TRIG_ONCE', 'CHECKPOINT': 'TRIG_CHECKPOINT', 'EXIT': 'TRIG_EXIT', 'BANTER': 'TRIG_BANTER'}

def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'

def emit(levels):
    out = ['/* GENERATED by tools/maps/make_maps.py - edit that, not this. */',
           '#include "maps.h"', '#include "dialog.h"', '']
    names = []
    for i, m in enumerate(levels):
        p = 'L%d' % (i + 1)
        out.append('static const char *const %s_ROWS[] = {' % p)
        for r in m.rows(): out.append('    %s,' % cstr(r))
        out.append('};')
        out.append('static const MapTrig %s_TRIG[] = {' % p)
        for t in m.trig:
            fl = '|'.join(FLAGS[f] for f in t[5].split('|'))
            out.append('    { %d, %d, %d, %d, %s, %s },' % (t[0], t[1], t[2], t[3], t[4], fl))
        out.append('    { 0, 0, 0, 0, 0, 0 }\n};')
        out.append('static const MapGroup %s_GROUP[] = {' % p)
        for g in m.group: out.append('    { %d, %s, %d, %d, %d, %d },' % ((m.gate_index(g[0], g[1]),) + g[2:]))
        out.append('    { -1, 0, 0, 0, 0, 0 }\n};')
        out.append('static const char *const %s_SIGN[] = {' % p)
        for s in sorted(m.sign): out.append('    %s,' % cstr(s[2]))
        out.append('    0\n};')
        out.append('static const int8_t %s_PIPE[] = { %s };' % (p, ', '.join(map(str, m.pipe + [0]))))
        out.append('static const int8_t %s_ORGAN[] = { %s };' % (p, ', '.join(map(str, list(m.organ) + [0]))))
        out.append('static const MapHook %s_HOOK[] = {' % p)
        for h in m.hook: out.append('    { %d, %d, %d, %d },' % h)
        out.append('    { 0, 0, 0, 0 }\n};')
        out.append('static const MapBoulder %s_BOUL[] = {' % p)
        for b in m.boul: out.append('    { %d, %d, %d, %d, %d, %d, %d, %d },' % b)
        out.append('    { 0, 0, 0, 0, 0, 0, 0, 0 }\n};\n')
        names.append((p, m))
    out.append('const LevelData LEVELS[NUM_LEVELS] = {')
    for p, m in names:
        out.append('    { %s, %s, %d, %d, %s, %s_ROWS, %s_TRIG, %d, %s_GROUP, %d, %s_SIGN, %d, %s_PIPE, %d, %s_ORGAN, %d, %s_HOOK, %d, %s_BOUL, %d },'
                   % (cstr(m.name), cstr(m.sub), m.w, m.h, m.theme, p, p, len(m.trig), p, len(m.group), p, len(m.sign),
                      p, len(m.pipe), p, len(m.organ), p, len(m.hook), p, len(m.boul)))
    out.append('};')
    open(os.path.join(ROOT, 'source', 'maps.c'), 'w').write('\n'.join(out) + '\n')

COLORS = {
    '.': (60, 110, 50), ',': (120, 90, 60), ':': (110, 108, 104), '=': (150, 110, 70), '-': (120, 85, 55), '"': (140, 40, 40),
    '`': (200, 180, 130), "'": (150, 130, 95), '~': (30, 70, 110), 'w': (70, 120, 130), ' ': (0, 0, 0), '#': (70, 66, 64),
    'W': (90, 64, 44), 'U': (70, 50, 34), 'h': (30, 70, 30), '^': (200, 200, 210), '_': (160, 120, 90), 'p': (220, 200, 120), 'G': (40, 40, 40),
}

def preview(i, m):
    try:
        from PIL import Image
    except ImportError:
        return
    S = 6
    im = Image.new('RGB', (m.w * S, m.h * S))
    px = im.load()
    for y in range(m.h):
        for x in range(m.w):
            c = m.g[y][x]
            col = COLORS.get(c)
            if col is None:
                col = (255, 60, 200) if c in '<>}{96 78' else ((255, 220, 40) if c in '$12345' else ((40, 255, 255) if c in '/)' else (230, 140, 40)))
            for j in range(S):
                for k in range(S):
                    px[x * S + k, y * S + j] = col
    os.makedirs(os.path.join(ROOT, 'tools', 'out'), exist_ok=True)
    im.save(os.path.join(ROOT, 'tools', 'out', 'map_%d.png' % (i + 1)))

if __name__ == '__main__':
    levels = [level1(), level2(), level3(), level4()]
    emit(levels)
    for i, m in enumerate(levels):
        preview(i, m)
        if '-v' in sys.argv:
            print('\n'.join(m.rows()))
    print('maps written')
