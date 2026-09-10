# -*- coding: utf-8 -*-
# Engrosa los montantes/diagonales de las cerchas y agrega COLUMNAS macizas de
# acero que bajan del techo al piso (nada flotando). Material: acero (no hay
# textura de oxido en el proyecto).
# Ejecutar:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/_thick_pipes_cqb.py').read(), g, g)"
import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')
MAT_STEEL = (unreal.load_asset('/Game/Scene_Warehouse/VisualFramework/DemoRoom/Materials/M_Metal')
             or unreal.load_asset('/Game/Indoor_Shooting_Range/Materials/Metal_M'))

FLOOR_TOP = 2.0
BEAM_Z = 1960.0
made = {'thick': 0, 'cols': 0}


def setmat(a):
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc and MAT_STEEL:
        for i in range(max(1, smc.get_num_materials())):
            smc.set_material(i, MAT_STEEL)


# ---- 1. engrosar montantes/diagonales/correas de las cerchas (finos -> gruesos)
for a in EAS.get_all_level_actors():
    l = a.get_actor_label()
    if l.startswith(('CQB_Truss', 'CQB_BigTruss', 'CQB_Roof_Purlin', 'CQB_BigPurlin')):
        s = a.get_actor_scale3d()
        nx = 0.35 if s.x < 0.35 else s.x
        ny = s.y
        nz = 0.35 if s.z < 0.35 else s.z
        # el eje largo (>2) se deja; solo se engrosan los cortos
        if s.y < 0.35 and not (s.y > s.x and s.y > s.z):
            ny = 0.35
        a.set_actor_scale3d(unreal.Vector(nx, ny, nz))
        setmat(a)
        made['thick'] += 1

# ---- 2. columnas macizas floor->techo, en grilla que evita los pasillos abiertos
# la trinchera CQB va de x[-5400..5400] y[-4000..4000]; el galpon ~ +-6900
h = BEAM_Z - FLOOR_TOP
zs = h / 100.0
zc = FLOOR_TOP + h / 2.0
SIZE = 0.75           # 75 cm de lado
grid = []
for gx in (-4700.0, -2350.0, 0.0, 2350.0, 4700.0):
    for gy in (-3300.0, -1100.0, 1100.0, 3300.0):
        grid.append((gx, gy))
# perimetro extra pegado a los muros
for gx in (-6400.0, 6400.0):
    for gy in (-5200.0, -2600.0, 0.0, 2600.0, 5200.0):
        grid.append((gx, gy))
for gy in (-6400.0, 6400.0):
    for gx in (-5200.0, -2600.0, 0.0, 2600.0, 5200.0):
        grid.append((gx, gy))

# borrar columnas anteriores de este script
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith('CQB_Column_'):
        EAS.destroy_actor(a)

n = 0
for (gx, gy) in grid:
    x = EAS.spawn_actor_from_object(CUBE, unreal.Vector(gx, gy, zc), unreal.Rotator(0, 0, 0))
    x.set_actor_label('CQB_Column_%d' % n)
    x.set_actor_scale3d(unreal.Vector(SIZE, SIZE, zs))
    setmat(x)
    n += 1
made['cols'] = n

print('thick pipes / columns ->', made)
