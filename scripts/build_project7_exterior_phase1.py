import unreal

TAG = unreal.Name('P7_Exterior')
FOLDER = 'P7_Exterior/01_Industrial_Yard'

def asset(path):
    item = unreal.load_asset(path)
    if not item:
        raise RuntimeError('Missing asset: ' + path)
    return item

def mark(actor, label):
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.tags = [TAG]
    return actor

def mesh_actor(label, mesh, location, rotation=(0, 0, 0), scale=(1, 1, 1), material=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    mark(actor, label)
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    if material:
        comp.set_material(0, material)
    comp.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    comp.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

def clean_previous():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        try:
            if TAG in actor.tags:
                unreal.EditorLevelLibrary.destroy_actor(actor)
        except Exception:
            pass

try:
    clean_previous()
    cube = asset('/Engine/BasicShapes/Cube.Cube')
    ground = asset('/Game/DerelictCorridor/Materials/MaterialInstances/MI_SueloExterior_Detallado')
    gravel = asset('/Game/DerelictCorridor/Materials/MaterialInstances/MI_SueloExterior_Piedras')
    concrete = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/Surfaces/Urb_Wall_Plaster_Old_09/MI_Urb_Wall_Plaster_Old_09_A')
    rusty = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Urb_Road_Sheet_Metal_Rusty_03/MI_Urb_Road_Sheet_Metal_Rusty_03')
    rubble = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Ind_Rubble_Pile_Debris_Concrete_M_01/SM_Ind_Rubble_Pile_Debris_Concrete_M_01')
    gravel_pile = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Ind_Con_Pile_Rubble_Gravel_Patch_01/SM_Ind_Con_Pile_Rubble_Gravel_Patch_01')
    cabinet = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Urb_Road_Cabinet_Electric_Metal_01/SM_Urb_Road_Cabinet_Electric_Metal_01')
    box = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Urb_Street_ElectricalBox_Metal_Worn_01/SM_Urb_Street_ElectricalBox_Metal_Worn_01')
    road_sheet = asset('/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Urb_Road_Sheet_Metal_Rusty_03/SM_Urb_Road_Sheet_Metal_Rusty_03')

    # A grounded industrial forecourt connects naturally with the existing corridor.
    mesh_actor('P7 Yard - dirty concrete apron', cube, (8000, -5200, -22), scale=(92, 88, 0.18), material=ground)
    mesh_actor('P7 Yard - service road', cube, (8000, -6500, 0), scale=(30, 68, 0.10), material=gravel)
    mesh_actor('P7 Yard - left gravel shoulder', cube, (4100, -6500, 0), scale=(18, 68, 0.08), material=ground)
    mesh_actor('P7 Yard - right gravel shoulder', cube, (11900, -6500, 0), scale=(18, 68, 0.08), material=ground)

    # Perimeter and distant industrial mass remove the empty horizon.
    mesh_actor('P7 Perimeter - north wall', cube, (8000, -13200, 330), scale=(92, 1.2, 3.4), material=concrete)
    mesh_actor('P7 Perimeter - west wall', cube, (-1100, -7000, 330), scale=(1.2, 64, 3.4), material=concrete)
    mesh_actor('P7 Perimeter - east wall', cube, (17100, -7000, 330), scale=(1.2, 64, 3.4), material=concrete)
    mesh_actor('P7 Distant plant - left', cube, (2600, -11600, 750), scale=(16, 8, 7.5), material=rusty)
    mesh_actor('P7 Distant plant - center', cube, (7900, -11800, 1020), scale=(22, 7, 10.2), material=concrete)
    mesh_actor('P7 Distant plant - right', cube, (13700, -11300, 660), scale=(14, 9, 6.6), material=rusty)
    mesh_actor('P7 Loading barrier left', cube, (5600, -4500, 180), scale=(15, 0.6, 1.8), material=concrete)
    mesh_actor('P7 Loading barrier right', cube, (10400, -4500, 180), scale=(15, 0.6, 1.8), material=concrete)

    # Physical storytelling: repair cabinets, road plates, rubble and debris clusters.
    for index, location in enumerate([(6200, -4700, 0), (10200, -4900, 0), (4700, -7200, 0), (12800, -7600, 0), (7400, -9800, 0), (9300, -10400, 0)]):
        mesh_actor('P7 Rubble {}'.format(index + 1), rubble, location, rotation=(0, index * 48, 0), scale=(1.0 + index * .06, 1.0 + index * .06, 1.0))
    for index, location in enumerate([(6900, -5600, 0), (11300, -6300, 0), (5200, -8900, 0), (10100, -9300, 0)]):
        mesh_actor('P7 Gravel patch {}'.format(index + 1), gravel_pile, location, rotation=(0, index * 87, 0), scale=(1.2, 1.2, 1.0))
    for index, location in enumerate([(5850, -4720, 30), (10750, -4680, 30), (3000, -10600, 30), (14600, -10300, 30)]):
        mesh_actor('P7 Electrical cabinet {}'.format(index + 1), cabinet, location, rotation=(0, 90 if index % 2 else 0, 0), scale=(1.1, 1.1, 1.1))
    for index, location in enumerate([(6600, -6000, 15), (9400, -7600, 15), (5200, -8300, 15), (12500, -9000, 15)]):
        mesh_actor('P7 Street box {}'.format(index + 1), box, location, rotation=(0, index * 45, 0), scale=(1.0, 1.0, 1.0))
    for index, location in enumerate([(7550, -5700, 7), (8250, -7050, 7), (7100, -8450, 7), (8950, -9800, 7)]):
        mesh_actor('P7 Rusted road plate {}'.format(index + 1), road_sheet, location, rotation=(0, index * 28, 0), scale=(1.5, 1.5, 1.0))

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log('P7_PHASE1_COMPLETE: industrial yard shell, road, perimeter, distant plant and debris created.')
except Exception as error:
    unreal.log_error('P7_PHASE1_FAILED: {}'.format(error))
    raise
