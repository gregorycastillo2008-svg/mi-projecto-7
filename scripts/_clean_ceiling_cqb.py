# -*- coding: utf-8 -*-
# Limpia el techo de CQB_Wood_Houses: borra todas las cerchas, montantes,
# diagonales, correas y vigas cruzadas, y deja SOLO 20 vigas horizontales
# paralelas cruzando la nave. Conserva el faldon del techo, las luces y las
# columnas de piso.
# Ejecutar:
#   cfa execute_python --code "g=globals(); exec(open(r'D:/Unreal Projects/MyProject7/Scripts/_clean_ceiling_cqb.py').read(), g, g)"
import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')
MAT_STEEL = (unreal.load_asset('/Game/Scene_Warehouse/VisualFramework/DemoRoom/Materials/M_Metal')
             or unreal.load_asset('/Game/Indoor_Shooting_Range/Materials/Metal_M'))

# ---- 1. borrar toda la estructura sobrante del techo
KILL = ('CQB_Truss', 'CQB_BigTruss', 'CQB_Roof_Purlin', 'CQB_BigPurlin',
        'CQB_Warehouse_BeamX', 'CQB_Warehouse_BeamY', 'CQB_CeilBeam')
deleted = 0
for a in EAS.get_all_level_actors():
    l = a.get_actor_label()
    if any(l.startswith(k) for k in KILL):
        EAS.destroy_actor(a)
        deleted += 1

# ---- 2. dejar SOLO 20 vigas horizontales paralelas (a lo largo de X)
WX = 7200.0            # media longitud de la viga (cubre toda la nave en X)
Y0, Y1 = -6200.0, 6200.0
Z = 1900.0            # justo bajo el faldon del techo
N = 20
made = 0
for i in range(N):
    yy = Y0 + (Y1 - Y0) * (i / (N - 1))
    x = EAS.spawn_actor_from_object(CUBE, unreal.Vector(0.0, yy, Z), unreal.Rotator(0, 0, 0))
    x.set_actor_label('CQB_CeilBeam_%d' % i)
    x.set_actor_scale3d(unreal.Vector(2 * WX / 100.0, 0.40, 0.55))   # 40 cm ancho, 55 cm alto
    smc = x.get_component_by_class(unreal.StaticMeshComponent)
    if smc and MAT_STEEL:
        smc.set_material(0, MAT_STEEL)
    made += 1

print('clean ceiling -> borrados %d, vigas horizontales nuevas %d' % (deleted, made))
