import unreal

world = unreal.EditorLevelLibrary.get_editor_world()
actors = unreal.EditorLevelLibrary.get_all_level_actors()
unreal.log('P7_INSPECT_BEGIN actors={}'.format(len(actors)))
for actor in sorted(actors, key=lambda item: item.get_actor_label())[:400]:
    try:
        origin, extent = actor.get_actor_bounds(False)
        unreal.log('P7_ACTOR | {} | {} | origin={} extent={}'.format(
            actor.get_class().get_name(), actor.get_actor_label(), origin, extent))
    except Exception as error:
        unreal.log_warning('P7_SKIP {} {}'.format(actor.get_actor_label(), error))
unreal.log('P7_INSPECT_END')
