import unreal

center = unreal.Vector(4489, -4121, 164)
for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    try:
        origin, extent = actor.get_actor_bounds(False)
        if (origin - center).size() > 4500:
            continue
        components = actor.get_components_by_class(unreal.StaticMeshComponent)
        mesh_text = []
        for component in components[:3]:
            mesh = component.get_editor_property('static_mesh')
            mesh_text.append(mesh.get_path_name() if mesh else 'None')
        unreal.log('P7_NEAR | class={} | {} | loc={} | meshes={}'.format(
            actor.get_class().get_name(), actor.get_actor_label(), origin, mesh_text))
    except Exception:
        pass
unreal.log('P7_NEAR_END')
