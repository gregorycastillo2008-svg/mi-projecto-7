import unreal

FLOOR_Z = 0.0
ADJUST_PREFIXES = (
    'P7 Rubble',
    'P7 Gravel patch',
    'P7 Electrical cabinet',
    'P7 Street box',
    'P7 Rusted road plate',
)

adjusted = 0
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    try:
        label = actor.get_actor_label()
        if not label.startswith(ADJUST_PREFIXES):
            continue
        origin, extent = actor.get_actor_bounds(False)
        bottom = origin.z - extent.z
        actor.set_actor_location(
            unreal.Vector(origin.x, origin.y, origin.z + (FLOOR_Z - bottom)),
            False,
            True,
        )
        adjusted += 1
    except Exception as error:
        unreal.log_warning('P7 grounding skipped: {}'.format(error))

unreal.EditorLevelLibrary.save_current_level()
unreal.log('P7_GROUNDING_COMPLETE: {} exterior props aligned to floor Z={}.'.format(adjusted, FLOOR_Z))
