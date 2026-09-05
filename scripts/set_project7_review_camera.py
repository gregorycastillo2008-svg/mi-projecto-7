import unreal

camera = unreal.Vector(8000, -11200, 1600)
target = unreal.Vector(8000, -4400, 350)
rotation = unreal.MathLibrary.find_look_at_rotation(camera, target)
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(camera, rotation)
unreal.log('P7_REVIEW_CAMERA: {} {}'.format(camera, rotation))
