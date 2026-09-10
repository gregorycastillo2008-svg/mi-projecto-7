# -*- coding: utf-8 -*-
# Pasa TODA la estructura del galpon de CQB_Wood_Houses a acero: techo/vigas/columnas
# (estaban con material de madera), engrosa y endereza las columnas hasta el piso, y
# extiende cerchas de hierro por toda la nave (no solo el area de juego).
# Ejecutar:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/_steel_ceiling_cqb.py').read(), g, g)"
import unreal, math

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')
# metal "viejo" mas oscuro disponible en el proyecto (no hay textura de oxido real)
MAT_STEEL = (unreal.load_asset('/Game/Scene_Warehouse/VisualFramework/DemoRoom/Materials/M_Metal')
             or unreal.load_asset('/Game/Indoor_Shooting_Range/Materials/Metal_M'))

FLOOR_TOP = 2.0
BEAM_Z = 1960.0        # cota de las vigas del galpon
made = [0]

def setmat(a):
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc and MAT_STEEL:
        for i in range(max(1, smc.get_num_materials())):
            smc.set_material(i, MAT_STEEL)

def B(lbl, loc, sc, rot=(0.0, 0.0, 0.0)):
    x = EAS.spawn_actor_from_object(CUBE, unreal.Vector(*loc), unreal.Rotator(*rot))
    x.set_actor_label(lbl); x.set_actor_scale3d(unreal.Vector(*sc))
    setmat(x); made[0] += 1

# ---- 1. TODA la estructura del galpon -> acero ; columnas gruesas/rectas/al piso
for a in EAS.get_all_level_actors():
    l = a.get_actor_label()
    if l.startswith(('CQB_Warehouse_Roof', 'CQB_Warehouse_Beam', 'CQB_Warehouse_Wall')):
        setmat(a)
    elif l.startswith('CQB_Warehouse_Column'):
        p = a.get_actor_location()
        s = a.get_actor_scale3d()
        h = BEAM_Z - FLOOR_TOP                 # altura desde el piso hasta las vigas
        zs = h / 100.0
        a.set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
        a.set_actor_scale3d(unreal.Vector(0.60, 0.60, zs))   # 60 cm de lado
        a.set_actor_location(unreal.Vector(p.x, p.y, FLOOR_TOP + h / 2.0), False, False)
        setmat(a)
    elif l.startswith(('CQB_Roof', 'CQB_Truss')):
        setmat(a)                              # ya eran M_Metal; reasegurar

# ---- 2. CERCHAS DE HIERRO POR TODA LA NAVE (span completo ~140 m)
WX0, WX1 = -6900.0, 6900.0        # ancho util del galpon (X)
WHY = 6900.0                       # media luz en Y hasta el muro
EAVE_Z = 1650.0
RIDGE_Z = 1900.0
PITCH = math.degrees(math.atan2(RIDGE_Z - EAVE_Z, WHY))
LR = WHY / math.cos(math.radians(PITCH))
NT = 14
span_x = WX1 - WX0
for j in range(NT):
    xt = WX0 + (j + 0.5) * (span_x / NT)
    # cordon inferior
    B('CQB_BigTruss_%d_Tie' % j, (xt, 0.0, EAVE_Z + 6.0), (0.26, 2 * WHY / 100.0, 0.30))
    # cordones superiores
    RISE = WHY * math.tan(math.radians(PITCH))
    for sgn in (-1.0, 1.0):
        B('CQB_BigTruss_%d_Top_%d' % (j, int(sgn)),
          (xt, sgn * WHY / 2.0, EAVE_Z + RISE / 2.0 + 6.0),
          (0.24, LR / 100.0, 0.26), (sgn * -PITCH, 0.0, 0.0))
    # montante central
    B('CQB_BigTruss_%d_King' % j, (xt, 0.0, EAVE_Z + RISE / 2.0 + 6.0), (0.18, 0.18, RISE / 100.0))
    # diagonales
    for sgn in (-1.0, 1.0):
        y0, z0 = sgn * 0.5 * WHY, EAVE_Z + 6.0
        y1, z1 = 0.0, EAVE_Z + 6.0 + RISE
        L = math.hypot(y1 - y0, z1 - z0)
        ang = math.degrees(math.atan2(z1 - z0, (y1 - y0) if (y1 - y0) != 0 else 1e-3))
        B('CQB_BigTruss_%d_Web_%d' % (j, int(sgn)),
          (xt, (y0 + y1) / 2.0, (z0 + z1) / 2.0), (0.15, L / 100.0, 0.17), (ang - 90.0, 0.0, 0.0))

# ---- 3. correas de acero a lo largo, bajo el techo del galpon
NP = 22
for i in range(NP + 1):
    yy = -WHY + i * (2 * WHY / NP)
    B('CQB_BigPurlin_%d' % i, (0.0, yy, EAVE_Z + 40.0), ((WX1 - WX0) / 100.0, 0.12, 0.12))

print('steel ceiling ->', {'nuevos': made[0], 'material': MAT_STEEL.get_name() if MAT_STEEL else None})
