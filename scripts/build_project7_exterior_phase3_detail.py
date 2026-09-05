import unreal

TAG = unreal.Name('P7_Detail')
FOLDER = 'P7_Exterior/03_Dirt_Details_Life'

def mark(actor, label):
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.tags = [TAG]
    return actor

def cleanup():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        try:
            if TAG in actor.tags:
                unreal.EditorLevelLibrary.destroy_actor(actor)
        except Exception:
            pass

def static_mesh(label, path, location, rotation=(0,0,0), scale=(1,1,1), ground=False):
    mesh = unreal.load_asset(path)
    if not mesh:
        raise RuntimeError('Missing mesh ' + path)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    mark(actor, label)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    if ground:
        origin, extent = actor.get_actor_bounds(False)
        actor.set_actor_location(unreal.Vector(origin.x, origin.y, origin.z - extent.z), False, True)
    return actor

def decal(label, material_path, location, rotation, size):
    material = unreal.load_asset(material_path)
    if not material:
        raise RuntimeError('Missing decal material ' + material_path)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DecalActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    mark(actor, label)
    component = actor.get_component_by_class(unreal.DecalComponent)
    component.set_decal_material(material)
    component.set_editor_property('decal_size', unreal.Vector(*size))
    return actor

try:
    cleanup()
    fan = '/Game/DerelictCorridor/Assets/Custom/Fans/SM_WallFan_02'
    vent = '/Game/DerelictCorridor/Assets/Custom/Vents/SM_WallVent_01'
    cable_big = '/Game/DerelictCorridor/Assets/Custom/Cables/SM_Cables_01_Cables_Wall_Big'
    cable_medium = '/Game/DerelictCorridor/Assets/Custom/Cables/SM_Cables_01_Cables_Wall_Medium_03'
    rusty_sheet = '/Game/DerelictCorridor/Assets/Fab/Megascans/3D/Urb_Road_Sheet_Metal_Rusty_03/SM_Urb_Road_Sheet_Metal_Rusty_03'
    dusty_box = '/Game/Scene_Warehouse/Assets/MS/3D/Ind_Aba_Box_Dusty_Metal_Pristine_01/SM_Ind_Aba_Box_Dusty_Metal_Pristine_01'

    # Functional wall dressing — placed on the front service façade.
    for i, location in enumerate([(6500,-4070,620),(10300,-4070,620)]):
        static_mesh('P7 Ventilation fan {}'.format(i+1), fan, location, rotation=(0,90,0), scale=(1.5,1.5,1.5))
    for i, location in enumerate([(7200,-4075,420),(8850,-4075,430),(9700,-4075,410)]):
        static_mesh('P7 Service vent {}'.format(i+1), vent, location, rotation=(0,90,0), scale=(1.25,1.25,1.25))
    static_mesh('P7 Main hanging cable', cable_big, (7600,-4050,720), rotation=(0,90,0), scale=(1.5,1.5,1.5))
    static_mesh('P7 Auxiliary cable A', cable_medium, (8800,-4055,610), rotation=(0,90,0), scale=(1.4,1.4,1.4))
    static_mesh('P7 Auxiliary cable B', cable_medium, (9800,-4055,650), rotation=(0,90,0), scale=(1.35,1.35,1.35))

    # Ground clutter is physically grounded rather than floating.
    for i, location in enumerate([(6200,-5200,0),(7000,-6100,0),(11100,-5400,0),(11800,-7500,0)]):
        static_mesh('P7 Rusted discarded plate {}'.format(i+1), rusty_sheet, location, rotation=(0,i*38,0), scale=(1.25,1.25,1.0), ground=True)
    for i, location in enumerate([(5700,-6100,0),(10700,-6700,0),(4600,-8100,0),(12700,-9000,0)]):
        static_mesh('P7 Dusty utility box {}'.format(i+1), dusty_box, location, rotation=(0,i*70,0), scale=(1.0,1.0,1.0), ground=True)

    rust = '/Game/DerelictCorridor/Assets/Fab/Megascans/Decals/Ind_Decal_Leak_Rust_02/MI_Ind_Decal_Leak_Rust_02_A'
    concrete = '/Game/DerelictCorridor/Assets/Fab/Megascans/Decals/Ind_Decal_Leak_Concrete_03/MI_Ind_Decal_Leak_Concrete_03_A'
    rubble = '/Game/DerelictCorridor/Assets/Fab/Megascans/Decals/Nature_Decal_Pile_Rubble_02/MI_Nature_Decal_Pile_Rubble_02'
    # Decals remain in the map: grime, leaks and rubble buildup, not temporary particles.
    for i, location in enumerate([(6150,-4380,15),(7800,-4400,15),(9400,-4380,15),(10500,-4350,15)]):
        decal('P7 Rust leak {}'.format(i+1), rust, location, (0,90,0), (220,360,360))
    for i, location in enumerate([(5900,-5400,8),(7550,-6800,8),(9600,-7850,8),(11300,-9100,8)]):
        decal('P7 Concrete grime {}'.format(i+1), concrete, location, (0,-90,0), (120,420,420))
    for i, location in enumerate([(6400,-4900,8),(10200,-5700,8),(5000,-7800,8)]):
        decal('P7 Rubble decal {}'.format(i+1), rubble, location, (0,-90,i*35), (100,340,340))

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log('P7_PHASE3_COMPLETE: grounded service props, vents, cables and persistent dirt/rust/rubble decals saved.')
except Exception as error:
    unreal.log_error('P7_PHASE3_FAILED: {}'.format(error))
    raise
