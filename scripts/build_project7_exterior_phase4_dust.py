import unreal

TAG = unreal.Name('P7_Atmosphere')
FOLDER = 'P7_Exterior/04_Atmosphere'

def mark(actor, label):
    actor.set_actor_label(label)
    actor.set_folder_path(FOLDER)
    actor.tags = [TAG]
    return actor

def clear_previous():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        try:
            if TAG in actor.tags:
                unreal.EditorLevelLibrary.destroy_actor(actor)
        except Exception:
            pass

try:
    clear_previous()
    dust = unreal.load_asset('/Game/Variant_Horror/Blueprints/Light/Assets/NS_DustMote')
    if not dust:
        raise RuntimeError('NS_DustMote not found')
    for index, location in enumerate([(6500,-5200,330),(8200,-5700,420),(10100,-5200,350)]):
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.NiagaraActor, unreal.Vector(*location))
        mark(actor, 'P7 Ambient dust {}'.format(index + 1))
        component = actor.get_component_by_class(unreal.NiagaraComponent)
        component.set_asset(dust)
        component.set_editor_property('auto_activate', True)
        component.activate(True)
    unreal.EditorLevelLibrary.save_current_level()
    unreal.log('P7_PHASE4_COMPLETE: three low-density ambient dust volumes saved.')
except Exception as error:
    unreal.log_error('P7_PHASE4_FAILED: {}'.format(error))
    raise
