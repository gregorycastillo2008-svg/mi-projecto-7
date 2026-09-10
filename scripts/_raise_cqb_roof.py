# -*- coding: utf-8 -*-
# Levanta el galpon de CQB_Wood_Houses a ~20 m y reconstruye el techo de acero
# mucho mas alto. Ejecutar con:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/_raise_cqb_roof.py').read(), g, g)"
import unreal, math

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MAT_STEEL = (unreal.load_asset('/Game/Scene_Warehouse/VisualFramework/DemoRoom/Materials/M_Metal')
             or unreal.load_asset('/Game/CQB/Materials/M_CQB_BackroomsPaleConcrete'))
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')

# ---- 1. LEVANTAR EL GALPON ----
NEW_WALL_S = 20.0
NEW_WALL_Z = 1000.0
NEW_ROOF_Z = 2020.0
NEW_BEAM_Z = 1960.0
for a in EAS.get_all_level_actors():
    l = a.get_actor_label(); p = a.get_actor_location(); s = a.get_actor_scale3d()
    if 'Warehouse_Wall' in l:
        a.set_actor_location(unreal.Vector(p.x, p.y, NEW_WALL_Z), False, False)
        a.set_actor_scale3d(unreal.Vector(s.x, s.y, NEW_WALL_S))
    elif 'Warehouse_Column' in l:
        a.set_actor_location(unreal.Vector(p.x, p.y, NEW_WALL_Z - 30.0), False, False)
        a.set_actor_scale3d(unreal.Vector(s.x, s.y, NEW_WALL_S - 0.6))
    elif 'Warehouse_Roof' in l:
        a.set_actor_location(unreal.Vector(p.x, p.y, NEW_ROOF_Z), False, False)
    elif 'Warehouse_Beam' in l:
        a.set_actor_location(unreal.Vector(p.x, p.y, NEW_BEAM_Z), False, False)
    elif 'CeilingPanel' in l:
        a.set_actor_location(unreal.Vector(p.x, p.y, NEW_BEAM_Z - 40.0), False, False)

# ---- 2. RECONSTRUIR EL TECHO DE ACERO, MUCHO MAS ALTO ----
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith(('CQB_Roof', 'CQB_Truss')):
        EAS.destroy_actor(a)

AX0, AY0, AX1, AY1 = -5400.0, -4000.0, 5400.0, 4000.0
EAVE_Z = 1650.0
RIDGE_Z = 1900.0
ROOF_X = (AX1 - AX0) + 700.0
HALF_Y = (AY1 - AY0) / 2.0 + 250.0
PITCH = math.degrees(math.atan2(RIDGE_Z - EAVE_Z, HALF_Y))
SLOPE_L = HALF_Y / math.cos(math.radians(PITCH))
made = [0]


def B(lbl, loc, sc, rot=(0.0, 0.0, 0.0)):
    x = EAS.spawn_actor_from_object(CUBE, unreal.Vector(*loc), unreal.Rotator(*rot))
    x.set_actor_label(lbl)
    x.set_actor_scale3d(unreal.Vector(*sc))
    c = x.get_component_by_class(unreal.StaticMeshComponent)
    if c and MAT_STEEL:
        c.set_material(0, MAT_STEEL)
    made[0] += 1


for sgn in (-1.0, 1.0):
    B('CQB_Roof_Slope_%d' % int(sgn), (0.0, sgn * HALF_Y / 2.0, (EAVE_Z + RIDGE_Z) / 2.0),
      (ROOF_X / 100.0, SLOPE_L / 100.0, 0.05), (sgn * -PITCH, 0.0, 0.0))
B('CQB_Roof_Ridge', (0.0, 0.0, RIDGE_Z + 4.0), (ROOF_X / 100.0, 0.28, 0.30))
for i in range(17):
    xr = AX0 - 350.0 + i * (ROOF_X / 16)
    for sgn in (-1.0, 1.0):
        B('CQB_Roof_Purlin_%d_%d' % (i, int(sgn)), (xr, sgn * HALF_Y / 2.0, (EAVE_Z + RIDGE_Z) / 2.0 - 8.0),
          (0.10, SLOPE_L / 100.0, 0.10), (sgn * -PITCH, 0.0, 0.0))
HY_IN = (AY1 - AY0) / 2.0
RISE_IN = HY_IN * math.tan(math.radians(PITCH))
LR_IN = HY_IN / math.cos(math.radians(PITCH))
for j in range(10):
    xt = AX0 + (j + 0.5) * ((AX1 - AX0) / 10)
    B('CQB_Truss_%d_Tie' % j, (xt, 0.0, EAVE_Z + 6.0), (0.22, (AY1 - AY0) / 100.0, 0.26))
    for sgn in (-1.0, 1.0):
        B('CQB_Truss_%d_Top_%d' % (j, int(sgn)), (xt, sgn * HY_IN / 2.0, EAVE_Z + RISE_IN / 2.0 + 6.0),
          (0.20, LR_IN / 100.0, 0.22), (sgn * -PITCH, 0.0, 0.0))
    B('CQB_Truss_%d_King' % j, (xt, 0.0, EAVE_Z + RISE_IN / 2.0 + 6.0), (0.16, 0.16, RISE_IN / 100.0))
    for sgn in (-1.0, 1.0):
        y0 = sgn * 0.5 * HY_IN; z0 = EAVE_Z + 6.0
        y1 = 0.0; z1 = EAVE_Z + 6.0 + RISE_IN
        L = math.hypot(y1 - y0, z1 - z0)
        ang = math.degrees(math.atan2(z1 - z0, (y1 - y0) if (y1 - y0) != 0 else 1e-3))
        B('CQB_Truss_%d_Web_%d' % (j, int(sgn)), (xt, (y0 + y1) / 2.0, (z0 + z1) / 2.0),
          (0.13, L / 100.0, 0.15), (ang - 90.0, 0.0, 0.0))

# ---- 3. subir las luces ----
for a in EAS.get_all_level_actors():
    if 'CeilingLight' in a.get_actor_label():
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, EAVE_Z - 140.0), False, False)

print('OK -> galpon ~20 m, techo acero EAVE %d / RIDGE %d, pitch %.1f, actores nuevos %d'
      % (EAVE_Z, RIDGE_Z, PITCH, made[0]))
