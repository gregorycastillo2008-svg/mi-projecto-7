# -*- coding: utf-8 -*-
# Reconstruye el interior de CQB_Wood_Houses para que se parezca a la foto de
# referencia: gran nave con TECHO ALTO de cerchas de acero oscuro (~9 m), planta
# baja tipo laberinto CQB en tablero OSB, y varias CASAS DE 2 PISOS bien
# configuradas (habitaciones con puertas, losa, muros, barandas y ESCALERA RECTA
# real), con una pasarela elevada uniendo dos de ellas.
# Determinista. Ejecutar dentro del editor con namespace unico:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/build_cqb_bodycam.py').read(), g, g)"
import unreal, random, math

random.seed(7)

CUBE      = unreal.load_asset('/Engine/BasicShapes/Cube')
MAT_WOOD  = unreal.load_asset('/Game/CQB/Materials/M_CQB_Wood_Professional')
MAT_PALE  = unreal.load_asset('/Game/CQB/Materials/M_CQB_BackroomsPaleConcrete')
MAT_CONC  = unreal.load_asset('/Game/CQB/Materials/M_CQB_DirtyConcrete') or MAT_PALE
MAT_STEEL = (unreal.load_asset('/Game/Scene_Warehouse/VisualFramework/DemoRoom/Materials/M_Metal')
             or unreal.load_asset('/Game/Indoor_Shooting_Range/Materials/Metal_M') or MAT_PALE)
EAS       = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# ---------------------------------------------------------------- parametros
AX0, AY0, AX1, AY1 = -5400.0, -4000.0, 5400.0, 4000.0   # footprint del edificio
MINROOM = 1050.0
MAXROOM = 2250.0
GAP     = 240.0
HALF_P  = 0.20
THICK   = 0.15
FULL_Z, FULL_S = 135.0, 2.70          # muro planta baja: 2,70 m (OSB, estilo foto)
HALF_Z, HALF_S = 105.0, 2.10          # muro media altura / baranda maciza
WIN_CAP = 6
FRAME_C = (4500.0, 3100.0)
PLAT_RECT = (3050.0, -1650.0, 5400.0, 1650.0)

# techo: cercha de acero, gablete MUY tendido, alto (nave ~9,9 m util)
EAVE_Z  = 815.0                       # cota del cordon inferior (nivel de alero)
RIDGE_Z = 965.0                       # cota de la cumbrera
ROOF_X  = (AX1 - AX0) + 700.0
HALF_Y  = (AY1 - AY0) / 2.0 + 250.0   # media luz hasta el alero
PITCH   = math.degrees(math.atan2(RIDGE_Z - EAVE_Z, HALF_Y))
SLOPE_L = HALF_Y / math.cos(math.radians(PITCH))

# casas de 2 piso: cuadradas de 6 m de lado, bien separadas
LEVEL2 = [(-3400.0, -1900.0), (-800.0, 1700.0), (2100.0, -1500.0), (3700.0, 1600.0)]
L2_HALF = 300.0                       # media planta (6 m de lado)
L2_WS   = 2.55                        # altura de muro por planta
L2_SLAB = 265.0                       # cota de la losa del 2o piso (borde superior planta baja)
DOOR_W  = 150.0                       # ancho de vano de puerta

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

# ---------------------------------------------------------------- 1. borrar lo viejo
KEEP = ('Warehouse_Wall', 'Warehouse_Column', 'Warehouse_Beam', 'Warehouse_Roof',
        'CQB_Floor', 'CeilingLight', 'CeilingPanel')
deleted = 0
for a in EAS.get_all_level_actors():
    lbl = a.get_actor_label()
    if not lbl.startswith('CQB_'):
        continue
    if any(k in lbl for k in KEEP):
        continue
    EAS.destroy_actor(a)
    deleted += 1

for a in EAS.get_all_level_actors():
    try:
        a.set_is_temporarily_hidden_in_editor(False)
    except Exception:
        pass

# ---------------------------------------------------------------- 2. suelo de hormigon gris
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith('CQB_Floor'):
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc and MAT_CONC:
            smc.set_material(0, MAT_CONC)

# ---------------------------------------------------------------- 3. tramo de muro
def wall_seg(cx, cy, length, axis, half, allow_window=True):
    if length < 60.0:
        return
    if _in(cx, cy, PLAT_RECT):
        half = True
        allow_window = False
    z, s = (HALF_Z, HALF_S) if half else (FULL_Z, FULL_S)
    sc = (length / 100.0, THICK, s) if axis == 'x' else (THICK, length / 100.0, s)
    if (allow_window and not half and length > 640.0 and win_left[0] > 0
            and random.random() < 0.28):
        win_left[0] -= 1
        side = (length - DOOR_W) / 2.0
        for sgn in (-1.0, 1.0):
            o = sgn * (DOOR_W / 2.0 + side / 2.0)
            bx = cx + (o if axis == 'x' else 0.0)
            by = cy + (o if axis == 'y' else 0.0)
            box('CQB_Maze_%d' % nid(), (bx, by, FULL_Z),
                (side / 100.0, THICK, FULL_S) if axis == 'x' else (THICK, side / 100.0, FULL_S))
        # dintel sobre el vano
        box('CQB_Maze_%d' % nid(), (cx, cy, 235.0),
            (DOOR_W / 100.0, THICK, 0.70) if axis == 'x' else (THICK, DOOR_W / 100.0, 0.70))
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

# ---------------------------------------------------------------- 4. BSP por cola
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

# ---------------------------------------------------------------- 5. detalle interior por sala
L2_KEEPOUT = [(cx, cy) for (cx, cy) in LEVEL2]
def near_house(cx, cy, m=700.0):
    return any(abs(cx - hx) < L2_HALF + m and abs(cy - hy) < L2_HALF + m for hx, hy in L2_KEEPOUT)

for (x0, y0, x1, y1) in rooms:
    cx, cy = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    if _in(cx, cy, PLAT_RECT) or near_house(cx, cy):
        continue
    w, h = x1 - x0, y1 - y0
    if max(w, h) < 1500.0 or random.random() > 0.5:
        continue
    half = random.random() < 0.40
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

# ---------------------------------------------------------------- 6. jaula de entramado
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

# ---------------------------------------------------------------- 7. helpers: baranda y escalera
def rail_run(p0, p1, fixed, axis, z, tag='Rail'):
    L = abs(p1 - p0)
    if L < 40.0:
        return
    if axis == 'y':
        box('CQB_%s_%d' % (tag, nid()), (fixed, (p0 + p1) / 2.0, z + 96.0), (0.05, L / 100.0, 0.05), mat=MAT_WOOD)
    else:
        box('CQB_%s_%d' % (tag, nid()), ((p0 + p1) / 2.0, fixed, z + 96.0), (L / 100.0, 0.05, 0.05), mat=MAT_WOOD)
    n = max(1, int(L // 300))
    for k in range(n + 1):
        t = p0 + k * (L / n)
        px, py = (fixed, t) if axis == 'y' else (t, fixed)
        box('CQB_%s_%d' % (tag, nid()), (px, py, z + 50.0), (0.05, 0.05, 0.92), mat=MAT_WOOD)

def stair_run(x, y_bot, top_z, face_dir, tag):
    # escalera recta de peldanos de caja. face_dir = +1 sube hacia +Y, -1 hacia -Y
    STEPS = 14
    rise = top_z / STEPS
    tread = 30.0
    for i in range(STEPS):
        z = rise * (i + 0.5)
        y = y_bot + face_dir * (tread * (i + 0.5))
        box('CQB_%s_Step_%d' % (tag, i), (x, y, z / 2.0), (2.0, tread / 100.0, z / 100.0), mat=MAT_WOOD)
    # zancas laterales + baranda
    run = tread * STEPS
    for sgn in (-1.0, 1.0):
        box('CQB_%s_Stringer_%d' % (tag, int(sgn)),
            (x + sgn * 105.0, y_bot + face_dir * run / 2.0, top_z / 2.0 + 8.0),
            (0.10, run / 100.0, 0.14), rot=(-face_dir * math.degrees(math.atan2(top_z, run)), 0.0, 0.0), mat=MAT_WOOD)
    rail_run(y_bot, y_bot + face_dir * run, x + 100.0, 'y', top_z * 0.5, tag=tag + 'R')

# ---------------------------------------------------------------- 8. plataforma + escalera + baranda
DECK_Z = 152.0
box('CQB_Plat_Deck', (4400.0, 0.0, DECK_Z), (24.0, 32.0, 0.28), mat=MAT_WOOD)
for lx in (3300.0, 5400.0):
    for ly in (-1400.0, 0.0, 1400.0):
        box('CQB_Plat_Leg_%d' % nid(), (lx, ly, DECK_Z / 2.0), (0.14, 0.14, DECK_Z / 100.0 - 0.15), mat=MAT_WOOD)
stair_run(2980.0, -1750.0, DECK_Z, +1.0, 'PlatStair')
rail_run(-1550.0, 1550.0, 5400.0, 'y', DECK_Z, tag='PlatRail')
rail_run(3100.0, 5400.0, 1550.0, 'x', DECK_Z, tag='PlatRail')
rail_run(3100.0, 5400.0, -1550.0, 'x', DECK_Z, tag='PlatRail')

# ---------------------------------------------------------------- 9. grada + props
for i in range(4):
    box('CQB_Bleach_%d' % i, (5150.0, -3450.0 + i * 300.0, 15.0 * (i + 1)), (12.0, 3.0, 0.30 * (i + 1)), mat=MAT_WOOD)
for i in range(5):
    y = -3800.0 + i * 130.0
    box('CQB_Prop_ChairSeat_%d' % i, (-5150.0, y, 46.0), (0.45, 0.45, 0.06), mat=MAT_PALE)
    box('CQB_Prop_ChairBack_%d' % i, (-5330.0, y, 78.0), (0.06, 0.45, 0.58), mat=MAT_PALE)
box('CQB_Prop_Whiteboard', (-5385.0, -3200.0, 170.0), (0.05, 1.6, 1.1), mat=MAT_PALE)
# IBC tote / caja gris junto a un muro (como en la foto, abajo a la derecha)
box('CQB_Prop_Tote', (4650.0, -2650.0, 60.0), (1.15, 1.15, 1.20), mat=MAT_STEEL)
box('CQB_Prop_ToteCage', (4650.0, -2650.0, 60.0), (1.20, 1.20, 1.24), mat=MAT_STEEL)

# ---------------------------------------------------------------- 10. CASAS DE 2 PISOS bien configuradas
def house(idx, cx, cy):
    H = L2_HALF
    # --- losa de planta baja (borde) y losa del 2o piso
    box('CQB_L2_%d_SlabTop' % idx, (cx, cy, L2_SLAB), (2 * H / 100.0, 2 * H / 100.0, 0.20), mat=MAT_WOOD)

    def perim(z0, ws, with_upper_door):
        zc = z0 + ws * 50.0
        # 4 muros de perimetro; puerta (vano) en -Y siempre, y en +X en la planta alta
        # -Y con vano central
        seg = (2 * H - DOOR_W) / 2.0
        off = DOOR_W / 2.0 + seg / 2.0
        for sgn in (-1.0, 1.0):
            box('CQB_L2_%d_W_%d_%d' % (idx, int(z0), int(sgn)),
                (cx + sgn * off, cy - H, zc), (seg / 100.0, THICK, ws), mat=MAT_WOOD)
        box('CQB_L2_%d_Lintel_%d' % (idx, int(z0)), (cx, cy - H, zc + ws * 32.0),
            (DOOR_W / 100.0, THICK, ws * 0.30), mat=MAT_WOOD)
        # +Y, -X macizos
        box('CQB_L2_%d_WpY_%d' % (idx, int(z0)), (cx, cy + H, zc), (2 * H / 100.0, THICK, ws), mat=MAT_WOOD)
        box('CQB_L2_%d_WnX_%d' % (idx, int(z0)), (cx - H, cy, zc), (THICK, 2 * H / 100.0, ws), mat=MAT_WOOD)
        # +X: macizo abajo, con vano arriba
        if with_upper_door:
            for sgn in (-1.0, 1.0):
                box('CQB_L2_%d_WpX_%d_%d' % (idx, int(z0), int(sgn)),
                    (cx + H, cy + sgn * off, zc), (THICK, seg / 100.0, ws), mat=MAT_WOOD)
        else:
            box('CQB_L2_%d_WpX_%d' % (idx, int(z0)), (cx + H, cy, zc), (THICK, 2 * H / 100.0, ws), mat=MAT_WOOD)

    perim(0.0, L2_WS, with_upper_door=False)                 # planta baja
    perim(L2_SLAB + 10.0, L2_WS, with_upper_door=True)       # planta alta

    # tabique interior de la planta baja (media divisoria con vano)
    box('CQB_L2_%d_Part' % idx, (cx + 60.0, cy - 60.0, L2_WS * 50.0), (THICK, (2 * H - 260.0) / 100.0, L2_WS), mat=MAT_WOOD)

    # --- baranda en el borde de la losa alta (3 lados; el 4o, +X, va a la pasarela)
    top = L2_SLAB + 10.0
    rail_run(cx - H, cx + H, cy + H, 'x', top, tag='L2Rail')
    rail_run(cy - H, cy + H, cx - H, 'y', top, tag='L2Rail')
    rail_run(cx - H, cx + H, cy - H, 'x', top, tag='L2Rail')

    # --- escalera recta exterior de subida por el lado -Y
    stair_run(cx, cy - H - 30.0, L2_SLAB + 10.0, -1.0, 'L2Stair%d' % idx)

for i, (cx, cy) in enumerate(LEVEL2):
    house(i, cx, cy)

# --- pasarela elevada uniendo casa 0 (-3400,-1900) con casa 2 (2100,-1500)
def catwalk(x0, y0, x1, y1, z):
    mx, my = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    L = math.hypot(x1 - x0, y1 - y0)
    ang = math.degrees(math.atan2(y1 - y0, x1 - x0))
    box('CQB_Catwalk_Deck', (mx, my, z), (L / 100.0, 1.4, 0.16), rot=(0.0, 0.0, ang), mat=MAT_WOOD)
    # barandas: aprox por tramos rectos a lo largo de X
    n = 6
    for k in range(n + 1):
        t = k / n
        px = x0 + (x1 - x0) * t
        py = y0 + (y1 - y0) * t
        for sgn in (-1.0, 1.0):
            box('CQB_Catwalk_Post_%d_%d' % (k, int(sgn)), (px, py + sgn * 70.0, z + 50.0), (0.05, 0.05, 0.92), mat=MAT_WOOD)
    for sgn in (-1.0, 1.0):
        box('CQB_Catwalk_Rail_%d' % int(sgn), (mx, my + sgn * 70.0, z + 96.0), (L / 100.0, 0.05, 0.05), rot=(0.0, 0.0, ang), mat=MAT_WOOD)

catwalk(LEVEL2[0][0] + L2_HALF, LEVEL2[0][1], LEVEL2[2][0] - L2_HALF, LEVEL2[2][1], L2_SLAB + 10.0)

# ---------------------------------------------------------------- 11. TECHO ALTO DE CERCHAS DE ACERO OSCURO
RIDGE_Y = 0.0
# faldones (un plano por agua), acero oscuro
for sgn in (-1.0, 1.0):
    box('CQB_Roof_Slope_%d' % int(sgn),
        (0.0, sgn * HALF_Y / 2.0, (EAVE_Z + RIDGE_Z) / 2.0),
        (ROOF_X / 100.0, SLOPE_L / 100.0, 0.05), rot=(sgn * -PITCH, 0.0, 0.0), mat=MAT_STEEL)
box('CQB_Roof_Ridge', (0.0, RIDGE_Y, RIDGE_Z + 4.0), (ROOF_X / 100.0, 0.28, 0.30), mat=MAT_STEEL)

# correas bajo cada faldon
NP = 16
for i in range(NP + 1):
    xr = AX0 - 350.0 + i * (ROOF_X / NP)
    for sgn in (-1.0, 1.0):
        box('CQB_Roof_Purlin_%d_%d' % (i, int(sgn)),
            (xr, sgn * HALF_Y / 2.0, (EAVE_Z + RIDGE_Z) / 2.0 - 8.0),
            (0.10, SLOPE_L / 100.0, 0.10), rot=(sgn * -PITCH, 0.0, 0.0), mat=MAT_STEEL)

# CERCHAS de acero: cordon inferior + montante central + diagonales (estilo foto)
NT = 10
HY_IN   = (AY1 - AY0) / 2.0
RISE_IN = HY_IN * math.tan(math.radians(PITCH))
LR_IN   = HY_IN / math.cos(math.radians(PITCH))
for j in range(NT):
    xt = AX0 + (j + 0.5) * ((AX1 - AX0) / NT)
    # cordon inferior (tie)
    box('CQB_Truss_%d_Tie' % j, (xt, 0.0, EAVE_Z + 6.0), (0.22, (AY1 - AY0) / 100.0, 0.26), mat=MAT_STEEL)
    # cordones superiores (uno por agua)
    for sgn in (-1.0, 1.0):
        box('CQB_Truss_%d_Top_%d' % (j, int(sgn)), (xt, sgn * HY_IN / 2.0, EAVE_Z + RISE_IN / 2.0 + 6.0),
            (0.20, LR_IN / 100.0, 0.22), rot=(sgn * -PITCH, 0.0, 0.0), mat=MAT_STEEL)
    # montante central (king post)
    box('CQB_Truss_%d_King' % j, (xt, 0.0, EAVE_Z + RISE_IN / 2.0 + 6.0), (0.16, 0.16, (RISE_IN) / 100.0), mat=MAT_STEEL)
    # diagonales en W a cada lado
    for sgn in (-1.0, 1.0):
        for frac0, frac1 in ((0.0, 0.5), (0.5, 1.0)):
            y0 = sgn * frac0 * HY_IN;  z0 = EAVE_Z + 6.0 + frac0 * RISE_IN
            y1 = sgn * frac1 * HY_IN;  z1 = EAVE_Z + 6.0 + (frac1 if frac1 < 1.0 else 1.0) * RISE_IN
            # alternar sentido para dar aspecto de W
            if frac0 == 0.0:
                y0, z0 = sgn * 0.5 * HY_IN, EAVE_Z + 6.0
            dy, dz = (y1 - y0), (z1 - z0)
            L = math.hypot(dy, dz)
            if L < 30.0:
                continue
            ang = math.degrees(math.atan2(dz, dy if dy != 0 else 1e-3))
            box('CQB_Truss_%d_Web_%d_%d' % (j, int(sgn), int(frac0 * 10)),
                (xt, (y0 + y1) / 2.0, (z0 + z1) / 2.0),
                (0.13, L / 100.0, 0.15), rot=(ang - 90.0, 0.0, 0.0), mat=MAT_STEEL)

# luces: dejarlas colgando bajo las cerchas
for a in EAS.get_all_level_actors():
    if 'CeilingLight' in a.get_actor_label():
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, EAVE_Z - 60.0), False, False)
    if 'CeilingPanel' in a.get_actor_label():
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, RIDGE_Z - 20.0), False, False)

# ---------------------------------------------------------------- 12. PlayerStart
for a in EAS.get_all_level_actors():
    if a.get_class().get_name() == 'PlayerStart':
        a.set_actor_location(unreal.Vector(0.0, -4650.0, 120.0), False, False)
        a.set_actor_rotation(unreal.Rotator(0.0, 0.0, 90.0), False)
        break

result = {'deleted': deleted, 'rooms': len(rooms), 'spawned': len(spawned),
          'houses': len(LEVEL2), 'eave_z': EAVE_Z, 'ridge_z': RIDGE_Z,
          'pitch_deg': round(PITCH, 1), 'windows': WIN_CAP - win_left[0],
          'errors': errors[:20]}
print('CQB build ->', result)
