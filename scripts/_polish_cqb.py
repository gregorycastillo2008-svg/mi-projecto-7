# -*- coding: utf-8 -*-
# Pulido de CQB_Wood_Houses: tablas mas gruesas, casas apoyadas al piso sin hueco
# (con losa de planta baja), y suelo con material de concreto realista.
# Ejecutar:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/_polish_cqb.py').read(), g, g)"
import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')
MAT_WOOD = unreal.load_asset('/Game/CQB/Materials/M_CQB_Wood_Professional')
MAT_FLOOR = (unreal.load_asset('/Game/Scene_Warehouse/Materials/Bodycam/M_BodycamConcreteReliable')
             or unreal.load_asset('/Game/Indoor_Shooting_Range/Materials/Concrete_Smooth_M')
             or unreal.load_asset('/Game/CQB/Materials/M_CQB_DirtyConcrete'))

THICK = 0.30          # antes 0.15 -> tablas mas gruesas
RAIL_T = 0.10         # antes 0.05/0.06
FLOOR_Z = -3.0        # top ~ +2 -> los muros (bottom 0) apoyan sin hueco

changed = {'thick': 0, 'rail': 0, 'floor': 0, 'slab': 0}

# ---- 1. SUELO: material realista + subir para que no quede hueco bajo los muros
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith('CQB_Floor'):
        p = a.get_actor_location()
        a.set_actor_location(unreal.Vector(p.x, p.y, FLOOR_Z), False, False)
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc and MAT_FLOOR:
            for i in range(max(1, smc.get_num_materials())):
                smc.set_material(i, MAT_FLOOR)
        changed['floor'] += 1

# ---- 2. TABLAS MAS GRUESAS: engrosar el eje fino de cada muro
WALL_PREFIXES = ('CQB_Maze_', 'CQB_L2_', 'CQB_Catwalk_Deck', 'CQB_Plat_Deck', 'CQB_Plat_Ramp')
RAIL_PREFIXES = ('CQB_Plat_Rail', 'CQB_Plat_Post', 'CQB_L2Rail', 'CQB_L2StairR', 'CQB_PlatStairR',
                 'CQB_Catwalk_Post', 'CQB_Catwalk_Rail', 'CQB_Frame_')
for a in EAS.get_all_level_actors():
    l = a.get_actor_label()
    s = a.get_actor_scale3d()
    if any(l.startswith(p) for p in RAIL_PREFIXES):
        nx = RAIL_T if s.x < 0.25 else s.x
        ny = RAIL_T if s.y < 0.25 else s.y
        nz = RAIL_T if (s.z < 0.25 and 'Post' not in l and 'Frame' not in l) else s.z
        if (nx, ny, nz) != (s.x, s.y, s.z):
            a.set_actor_scale3d(unreal.Vector(nx, ny, nz)); changed['rail'] += 1
        continue
    if any(l.startswith(p) for p in WALL_PREFIXES):
        nx, ny = s.x, s.y
        if 0.0 < s.x < 0.5 and s.x <= s.y:
            nx = THICK
        elif 0.0 < s.y < 0.5 and s.y < s.x:
            ny = THICK
        if (nx, ny) != (s.x, s.y):
            a.set_actor_scale3d(unreal.Vector(nx, ny, s.z)); changed['thick'] += 1

# ---- 3. CASAS: losa de planta baja para que no se vea el hueco / apoyo perfecto
LEVEL2 = [(-3400.0, -1900.0), (-800.0, 1700.0), (2100.0, -1500.0), (3700.0, 1600.0)]
L2_HALF = 300.0
# borrar losas de planta baja previas
for a in EAS.get_all_level_actors():
    if a.get_actor_label().startswith('CQB_L2_') and 'FloorSlab' in a.get_actor_label():
        EAS.destroy_actor(a)
for idx, (cx, cy) in enumerate(LEVEL2):
    x = EAS.spawn_actor_from_object(CUBE, unreal.Vector(cx, cy, 6.0), unreal.Rotator(0, 0, 0))
    x.set_actor_label('CQB_L2_%d_FloorSlab' % idx)
    x.set_actor_scale3d(unreal.Vector(2 * L2_HALF / 100.0, 2 * L2_HALF / 100.0, 0.14))
    c = x.get_component_by_class(unreal.StaticMeshComponent)
    if c and MAT_WOOD:
        c.set_material(0, MAT_WOOD)
    changed['slab'] += 1

print('polish CQB ->', changed)
