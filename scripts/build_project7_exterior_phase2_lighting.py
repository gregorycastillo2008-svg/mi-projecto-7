import unreal

TAG = unreal.Name('P7_Lighting')
FOLDER = 'P7_Exterior/02_Lighting_Atmosphere'

def mark(actor, label):
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.tags = [TAG]
    return actor

def set_prop(target, name, value):
    try:
        target.set_editor_property(name, value)
    except Exception as error:
        unreal.log_warning('P7 lighting property skipped {}: {}'.format(name, error))

def light_component(actor, component_class):
    component = actor.get_component_by_class(component_class)
    if not component:
        raise RuntimeError('Light component missing on {}'.format(actor.get_actor_label()))
    return component

def destroy_previous():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        try:
            if TAG in actor.tags:
                unreal.EditorLevelLibrary.destroy_actor(actor)
        except Exception:
            pass

def point_light(label, location, color, intensity, radius, temperature=None):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*location))
    mark(actor, label)
    light = light_component(actor, unreal.PointLightComponent)
    set_prop(light, 'intensity', intensity)
    set_prop(light, 'attenuation_radius', radius)
    set_prop(light, 'light_color', unreal.Color(int(color[0] * 255), int(color[1] * 255), int(color[2] * 255), 255))
    set_prop(light, 'cast_shadows', True)
    set_prop(light, 'use_inverse_squared_falloff', True)
    if temperature:
        set_prop(light, 'use_temperature', True)
        set_prop(light, 'temperature', temperature)
    return actor

def static_mesh(label, mesh_path, location, rotation=(0,0,0), scale=(1,1,1)):
    mesh = unreal.load_asset(mesh_path)
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    mark(actor, label)
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

try:
    destroy_previous()
    # Use the level's existing sun. A second directional light makes fog and
    # forward shading unstable, so only create one if the level has none.
    sun = next((actor for actor in unreal.EditorLevelLibrary.get_all_level_actors()
                if actor.get_class().get_name() == 'DirectionalLight'), None)
    if not sun:
        sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,0), unreal.Rotator(-38, 132, 0))
        mark(sun, 'P7 Sun - warm late afternoon')
    sun_component = light_component(sun, unreal.DirectionalLightComponent)
    set_prop(sun_component, 'intensity', 18.0)
    set_prop(sun_component, 'light_color', unreal.Color(255, 210, 168, 255))
    set_prop(sun_component, 'cast_shadows', True)
    set_prop(sun_component, 'dynamic_shadow_distance_movable_light', 30000.0)

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(8000,-7000,2000))
    mark(sky, 'P7 Sky fill - cool industrial')
    sky_component = light_component(sky, unreal.SkyLightComponent)
    set_prop(sky_component, 'intensity', 1.35)
    set_prop(sky_component, 'light_color', unreal.Color(102, 145, 209, 255))
    try:
        sky_component.recapture_sky()
    except Exception:
        pass

    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(8000,-7000,0))
    mark(fog, 'P7 Atmosphere - subtle volumetric haze')
    fc = light_component(fog, unreal.ExponentialHeightFogComponent)
    set_prop(fc, 'fog_density', 0.008)
    set_prop(fc, 'fog_height_falloff', 0.22)
    set_prop(fc, 'start_distance', 1200.0)
    set_prop(fc, 'volumetric_fog_scattering_distribution', 0.65)
    set_prop(fc, 'volumetric_fog_extinction_scale', 0.65)

    # Warm loading dock lights contrast with the cool sky and make the entrance readable.
    fixture = '/Game/DerelictCorridor/Assets/Custom/LightFixture/SM_Light_Fixture_01_Emissive'
    for index, (location, color, intensity) in enumerate([
        ((6100,-4450,740), (1.0,0.50,0.18,1.0), 8500.0),
        ((7600,-4380,680), (1.0,0.59,0.27,1.0), 7000.0),
        ((9400,-4380,680), (1.0,0.59,0.27,1.0), 7000.0),
        ((10800,-4450,740), (1.0,0.50,0.18,1.0), 8500.0),
    ]):
        static_mesh('P7 Dock fixture {}'.format(index+1), fixture, location, rotation=(0,0,90), scale=(1.5,1.5,1.5))
        point_light('P7 Warm dock pool {}'.format(index+1), (location[0],location[1],location[2]-60), color, intensity, 1700.0, 3100)

    # Cold security pools define the street, avoiding a flat uniformly lit exterior.
    for index, location in enumerate([(4200,-7200,580),(11800,-7200,580),(5100,-10800,650),(11100,-10800,650)]):
        point_light('P7 Cool security pool {}'.format(index+1), location, (0.34,0.55,1.0,1.0), 7000.0, 2200.0, 6200)

    reflection = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SphereReflectionCapture, unreal.Vector(8000,-6700,500))
    mark(reflection, 'P7 Yard reflection capture')
    set_prop(light_component(reflection, unreal.SphereReflectionCaptureComponent), 'influence_radius', 9000.0)

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log('P7_PHASE2_COMPLETE: warm/cool exterior lighting, shadowed sun, skylight, volumetric haze and reflection coverage saved.')
except Exception as error:
    unreal.log_error('P7_PHASE2_FAILED: {}'.format(error))
    raise
