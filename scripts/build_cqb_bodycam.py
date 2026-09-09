# -*- coding: utf-8 -*-
# Reconstruye el interior de CQB_Wood_Houses: laberinto CQB de planta baja ALTA,
# con un GRAN techo a dos aguas de madera (cerchas king-post vistas, estilo foto)
# y varias casas de SEGUNDO PISO sobre la planta baja.
# Determinista. Ejecutar dentro del editor con namespace unico:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/build_cqb_bodycam.py').read(), g, g)"
import unreal, random, math

random.seed(7)

CUBE     = unreal.load_asset('/Engine/BasicShapes/Cube')
MAT_WOOD = unreal.load_asset('/Game/CQB/Materials/M_CQB_Wood_Professional')
MAT_PALE = unreal.load_asset('/Game/CQB/Materials/M_CQB_BackroomsPaleConcrete')
EAS      = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# ---------------------------------------------------------------- parametros
AX0, AY0, AX1, AY1 = -5400.0, -4000.0, 5400.0, 4000.0   # footprint del edificio
MINROOM = 1050.0
MAXROOM = 2250.0
GAP     = 240.0
HALF_P  = 0.22
THICK   = 0.15
FULL_Z, FULL_S = 225.0, 4.50          # muro planta baja: 4,5 m
HALF_Z, HALF_S = 120.0, 2.40          # muro media altura
WIN_CAP = 7
FRAME_C = (4500.0, 3100.0)
PLAT_RECT = (3050.0, -1650.0, 5400.0, 1650.0)

# techo
EAVE_Z = 480.0
PITCH  = 26.0
OV     = 350.0                        # vuelo de alero
RAD    = math.radians(PITCH)
SPAN   = (AY1 - AY0) / 2.0 + OV       # media luz hasta el alero
RISE   = SPAN * math.tan(RAD)
RIDGE_Z = EAVE_Z + RISE
SLOPE_L = SPAN / math.cos(RAD)
ROOF_X  = (AX1 - AX0) + 2 * OV
HY_IN   = (AY1 - AY0) / 2.0           # media luz interior (cercha, sin vuelo)
RISE_IN = HY_IN * math.tan(RAD)
LR_IN   = HY_IN / math.cos(RAD)

# casas de 2o piso (centros; |cy|<2500 para que quepan bajo el techo)
LEVEL2 = [(-3600.0, -1100.0), (-1000.0, 1600.0), (1500.0, -1500.0),
          (3600.0, 1100.0), (-3200.0, 2100.0), (2900.0, -300.0)]
L2_H  = 1200.0        # media planta de la casa de arriba (2400 de lado)
L2_FZ = 452.0         # cota de la losa del 2o piso
L2_WS = 2.70          # altura de muro del 2o piso

_id = [0]
def nid():
    _id[0] += 1
    return _id[0]

errors = []
spawned = []
win_left = [WIN_CAP]

def box(label, loc, scale, rot=(0.0, 0.0, 0.0), mat=MAT_WOOD):
    try:
        a = EAS.spawn_actor_from_object(CUBE, unreal.Vector(*loc),
                                        unreal.Rotator(rot[0], rot[1], rot[2]))
        a.set_actor_label(label)
        a.set_actor_scale3d(unreal.Vector(*scale))
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc and mat:
            smc.set_material(0, mat)
        spawned.append(label)
        return a
    except Exception as e:
        errors.append('%s: %s' % (label, e))
        return None

def snap(v):
    return round(v / 300.0) * 300.0

def _in(cx, cy, r):
    return r[0] < cx < r[2] and r[1] < cy < r[3]

# ---------------------------------------------------------------- 1. borrar lo viejo (incluido el techo plano de galpon)
KEEP = ('Warehouse_Wall', 'Warehouse_Column', 'CQB_Floor', 'CeilingLight')
deleted = 0
for a in EAS.get_all_level_actors():
    lbl = a.get_actor_label()
    if not lbl.startswith('CQB_'):
        continue
    keep = False
    for k in KEEP:
        if k in lbl:
            keep = True
            break
    if not keep:
        EAS.destroy_actor(a)
        deleted += 1

for a in EAS.get_all_level_actors():
    try:
        a.set_is_temporarily_hidden_in_editor(False)
    except Exception:
        pass

# ---------------------------------------------------------------- 2. tramo de muro
def wall_seg(cx, cy, length, axis, half, allow_window=True):
    if length < 60.0:
        return
    if _in(cx, cy, PLAT_RECT):
        half = True
        allow_window = False
    z, s = (HALF_Z, HALF_S) if half else (FULL_Z, FULL_S)
    sc = (length / 100.0, THICK, s) if axis == 'x' else (THICK, length / 100.0, s)
    if (allow_window and not half and length > 620.0 and win_left[0] > 0
            and random.random() < 0.30):
        win_left[0] -= 1
        side = (length - 300.0) / 2.0
        for sgn in (-1.0, 1.0):
            o = sgn * (150.0 + side / 2.0)
            bx = cx + (o if axis == 'x' else 0.0)
            by = cy + (o if axis == 'y' else 0.0)
            box('CQB_Maze_%d' % nid(), (bx, by, FULL_Z),
                (side / 100.0, THICK, FULL_S) if axis == 'x' else (THICK, side / 100.0, FULL_S))
        for zz, hh in ((115.0, 0.30), (330.0, 0.34)):
            box('CQB_Maze_%d' % nid(), (cx, cy, zz),
                (3.0, THICK, hh) if axis == 'x' else (THICK, 3.0, hh))
        return
    box('CQB_Maze_%d' % nid(), (cx, cy, z), sc)

def hwall(x0, x1, y, xg, half=False):
    g0, g1 = xg - GAP / 2.0, xg + GAP / 2.0
    for a, b in ((x0, g0), (g1, x1)):
        if b - a > 40.0:
            wall_seg((a + b) / 2.0, y, b - a, 'x', half)

def vwall(y0, y1, x, yg, half=False):
    g0, g1 = yg - GAP / 2.0, yg + GAP / 2.0
    for a, b in ((y0, g0), (g1, y1)):
        if b - a > 40.0:
            wall_seg(x, (a + b) / 2.0, b - a, 'y', half)

# ---------------------------------------------------------------- 3. BSP por cola
rooms = []
queue = [(AX0, AY0, AX1, AY1)]
while queue:
    x0, y0, x1, y1 = queue.pop(0)
    w, h = x1 - x0, y1 - y0
    if not ((w > MAXROOM or h > MAXROOM) and (w > 2 * MINROOM or h > 2 * MINROOM)):
        rooms.append((x0, y0, x1, y1))
        continue
    split_v = w >= h
    if abs(w - h) < 900.0:
        split_v = random.random() < 0.5
    if split_v and w <= 2 * MINROOM:
        split_v = False
    if (not split_v) and h <= 2 * MINROOM:
        split_v = True
    half = random.random() < HALF_P
    if split_v:
        xs = snap(random.uniform(x0 + MINROOM, x1 - MINROOM))
        yg = snap(random.uniform(y0 + GAP, y1 - GAP))
        vwall(y0, y1, xs, yg, half)
        queue.append((x0, y0, xs, y1)); queue.append((xs, y0, x1, y1))
    else:
        ys = snap(random.uniform(y0 + MINROOM, y1 - MINROOM))
        xg = snap(random.uniform(x0 + GAP, x1 - GAP))
        hwall(x0, x1, ys, xg, half)
        queue.append((x0, y0, x1, ys)); queue.append((x0, ys, x1, y1))

hwall(AX0, AX1, AY1, snap(random.uniform(-2400, 2400)))
hwall(AX0, AX1, AY0, 0.0)
vwall(AY0, AY1, AX1, snap(random.uniform(-2000, 2000)))
vwall(AY0, AY1, AX0, snap(random.uniform(-2000, 2000)))

# ---------------------------------------------------------------- 4. detalle interior por sala
for (x0, y0, x1, y1) in rooms:
    cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    if _in(cx, cy, PLAT_RECT):
        continue
    w, h = x1 - x0, y1 - y0
    if max(w, h) < 1500.0 or random.random() > 0.55:
        continue
    half = random.random() < 0.45
    if w >= h:
        px = snap(random.uniform(x0 + 400.0, x1 - 400.0))
        seglen = h * random.uniform(0.42, 0.72)
        anchor = y0 if random.random() < 0.5 else y1
        cyy = anchor + (seglen / 2.0 if anchor == y0 else -seglen / 2.0)
        wall_seg(px, cyy, seglen, 'y', half, allow_window=False)
    else:
        py = snap(random.uniform(y0 + 400.0, y1 - 400.0))
        seglen = w * random.uniform(0.42, 0.72)
        anchor = x0 if random.random() < 0.5 else x1
        cxx = anchor + (seglen / 2.0 if anchor == x0 else -seglen / 2.0)
        wall_seg(cxx, py, seglen, 'x', half, allow_window=False)

# ---------------------------------------------------------------- 5. jaula de entramado
fx, fy = FRAME_C
R = 200.0
studs = int(2 * R // 40)
for k in range(studs + 1):
    t = -R + k * (2 * R / studs)
    box('CQB_Frame_%d' % nid(), (fx - R, fy + t, FULL_Z), (0.09, 0.09, FULL_S))
    box('CQB_Frame_%d' % nid(), (fx + R, fy + t, FULL_Z), (0.09, 0.09, FULL_S))
    box('CQB_Frame_%d' % nid(), (fx + t, fy + R, FULL_Z), (0.09, 0.09, FULL_S))
    if abs(t) > 130.0:
        box('CQB_Frame_%d' % nid(), (fx + t, fy - R, FULL_Z), (0.09, 0.09, FULL_S))
for sgn in (-1.0, 1.0):
    box('CQB_Frame_%d' % nid(), (fx, fy + sgn * R, FULL_Z + FULL_S * 50 - 8), (2 * R / 100.0, 0.10, 0.10))
    box('CQB_Frame_%d' % nid(), (fx + sgn * R, fy, FULL_Z + FULL_S * 50 - 8), (0.10, 2 * R / 100.0, 0.10))

# ---------------------------------------------------------------- 6. plataforma + rampa + baranda
DECK_Z = 152.0
box('CQB_Plat_Deck', (4400.0, 0.0, DECK_Z), (24.0, 32.0, 0.28))
for lx in (3300.0, 5400.0):
    for ly in (-1400.0, 0.0, 1400.0):
        box('CQB_Plat_Leg_%d' % nid(), (lx, ly, DECK_Z / 2.0), (0.14, 0.14, DECK_Z / 100.0 - 0.15))
box('CQB_Plat_Ramp', (2830.0, 0.0, 82.0), (19.0, 4.6, 0.16), rot=(0.0, -5.4, 0.0))
def rail_run(p0, p1, fixed, axis, z):
    L = abs(p1 - p0)
    if axis == 'y':
        box('CQB_Plat_Rail_%d' % nid(), (fixed, (p0 + p1) / 2.0, z + 78.0), (0.06, L / 100.0, 0.06))
    else:
        box('CQB_Plat_Rail_%d' % nid(), ((p0 + p1) / 2.0, fixed, z + 78.0), (L / 100.0, 0.06, 0.06))
    n = max(1, int(L // 320))
    for k in range(n + 1):
        t = p0 + k * (L / n)
        px, py = (fixed, t) if axis == 'y' else (t, fixed)
        box('CQB_Plat_Post_%d' % nid(), (px, py, z + 42.0), (0.06, 0.06, 0.72))
rail_run(-1550.0, 1550.0, 5400.0, 'y', DECK_Z)
rail_run(3100.0, 5400.0, 1550.0, 'x', DECK_Z)
rail_run(3100.0, 5400.0, -1550.0, 'x', DECK_Z)

# ---------------------------------------------------------------- 7. grada + props
for i in range(4):
    box('CQB_Bleach_%d' % i, (5150.0, -3450.0 + i * 300.0, 15.0 * (i + 1)), (12.0, 3.0, 0.30 * (i + 1)))
for i in range(5):
    y = -3800.0 + i * 130.0
    box('CQB_Prop_ChairSeat_%d' % i, (-5150.0, y, 46.0), (0.45, 0.45, 0.06))
    box('CQB_Prop_ChairBack_%d' % i, (-5330.0, y, 78.0), (0.06, 0.45, 0.58))
box('CQB_Prop_Whiteboard', (-5385.0, -3200.0, 170.0), (0.05, 1.6, 1.1), mat=MAT_PALE)

# ---------------------------------------------------------------- 8. casas de SEGUNDO PISO
for idx, (cx, cy) in enumerate(LEVEL2):
    box('CQB_L2_Slab_%d' % idx, (cx, cy, L2_FZ), (2 * L2_H / 100.0, 2 * L2_H / 100.0, 0.22))
    zc = L2_FZ + L2_WS * 50.0
    box('CQB_L2_WpX_%d' % idx, (cx + L2_H, cy, zc), (THICK, 2 * L2_H / 100.0, L2_WS))
    box('CQB_L2_WnX_%d' % idx, (cx - L2_H, cy, zc), (THICK, 2 * L2_H / 100.0, L2_WS))
    box('CQB_L2_WpY_%d' % idx, (cx, cy + L2_H, zc), (2 * L2_H / 100.0, THICK, L2_WS))
    seg = (2 * L2_H - 260.0) / 2.0
    off = 130.0 + seg / 2.0
    for sgn in (-1.0, 1.0):
        box('CQB_L2_WnY_%d_%d' % (idx, int(sgn)), (cx + sgn * off, cy - L2_H, zc), (seg / 100.0, THICK, L2_WS))
    # rampa de acceso por el lado -Y
    run = 1900.0
    ang = math.degrees(math.atan2(L2_FZ, run))
    box('CQB_L2_Ramp_%d' % idx, (cx, cy - L2_H - run / 2.0, L2_FZ / 2.0),
        (5.2, run / 100.0, 0.16), rot=(-ang, 0.0, 0.0))
    # barandilla en el borde de la losa (3 lados)
    rail_run(cx - L2_H, cx + L2_H, cy + L2_H, 'x', L2_FZ)
    rail_run(cy - L2_H, cy + L2_H, cx + L2_H, 'y', L2_FZ)
    rail_run(cy - L2_H, cy + L2_H, cx - L2_H, 'y', L2_FZ)

# ---------------------------------------------------------------- 9. GRAN TECHO A DOS AGUAS (cerchas king-post)
# faldones (un plano por agua)
for sgn in (-1.0, 1.0):
    box('CQB_Roof_Slope_%d' % int(sgn), (0.0, sgn * SPAN / 2.0, EAVE_Z + RISE / 2.0),
        (ROOF_X / 100.0, SLOPE_L / 100.0, 0.06), rot=(sgn * -PITCH, 0.0, 0.0))
# cumbrera
box('CQB_Roof_Ridge', (0.0, 0.0, RIDGE_Z - 6.0), (ROOF_X / 100.0, 0.30, 0.34))
# cabios / correas bajo cada faldon
NR = 20
for i in range(NR + 1):
    xr = AX0 - OV + i * (ROOF_X / NR)
    for sgn in (-1.0, 1.0):
        box('CQB_Roof_Rafter_%d_%d' % (i, int(sgn)),
            (xr, sgn * SPAN / 2.0, EAVE_Z + RISE / 2.0 - 9.0),
            (0.16, SLOPE_L / 100.0, 0.12), rot=(sgn * -PITCH, 0.0, 0.0))
# cerchas king-post
NT = 9
for j in range(NT):
    xt = AX0 + (j + 0.5) * ((AX1 - AX0) / NT)
    box('CQB_Truss_Tie_%d' % j, (xt, 0.0, EAVE_Z + 10.0), (0.30, (AY1 - AY0) / 100.0, 0.38))
    box('CQB_Truss_King_%d' % j, (xt, 0.0, EAVE_Z + RISE_IN / 2.0), (0.24, 0.24, (RISE_IN - 40.0) / 100.0))
    for sgn in (-1.0, 1.0):
        box('CQB_Truss_Raf_%d_%d' % (j, int(sgn)), (xt, sgn * HY_IN / 2.0, EAVE_Z + RISE_IN / 2.0),
            (0.24, LR_IN / 100.0, 0.30), rot=(sgn * -PITCH, 0.0, 0.0))
        # tornapunta: del king-post bajo a la mitad del cabio
        y1s, z1s = 0.0, EAVE_Z + 0.16 * RISE_IN
        y2s, z2s = sgn * 0.5 * HY_IN, EAVE_Z + 0.50 * RISE_IN
        dy, dz = (y2s - y1s), (z2s - z1s)
        L = math.hypot(dy, dz)
        a = math.degrees(math.atan2(dz, abs(dy)))
        box('CQB_Truss_Strut_%d_%d' % (j, int(sgn)),
            ((xt), (y1s + y2s) / 2.0, (z1s + z2s) / 2.0),
            (0.18, L / 100.0, 0.20), rot=(sgn * -a, 0.0, 0.0))

# luces: bajarlas para que cuelguen bajo el techo
for a in EAS.get_all_level_actors():
    if 'CeilingLight' in a.get_actor_label():
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, EAVE_Z - 30.0), False, False)

# ---------------------------------------------------------------- 10. PlayerStart
for a in EAS.get_all_level_actors():
    if a.get_class().get_name() == 'PlayerStart':
        a.set_actor_location(unreal.Vector(0.0, -4650.0, 120.0), False, False)
        a.set_actor_rotation(unreal.Rotator(0.0, 0.0, 90.0), False)
        break

result = {'deleted': deleted, 'rooms': len(rooms), 'spawned': len(spawned),
          'ridge_z': round(RIDGE_Z), 'windows': WIN_CAP - win_left[0], 'errors': errors[:20]}
print('CQB build ->', result)
