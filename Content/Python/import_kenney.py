"""Import Kenney FBX assets (blasters, cars, space) and place them in level.
Run inside UE5 editor: py /tmp/ue_import_kenney.py"""
import unreal
import os
import random

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_lib = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

base = os.path.expanduser("~/Downloads/kenney_models")
count = 0

# Import select blaster models (guns for weapon display)
blaster_dir = os.path.join(base, "blaster_kit/Models/FBX format")
blasters_to_import = ["blaster-h.fbx", "blaster-i.fbx", "blaster-j.fbx", "blaster-k.fbx",
                       "blaster-n.fbx", "blaster-o.fbx", "crate-wide.fbx", "target-large.fbx"]
for f in blasters_to_import:
    fp = os.path.join(blaster_dir, f)
    if os.path.exists(fp):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", fp)
        task.set_editor_property("destination_path", "/Game/AnalogStrike/ImportedAssets/Blasters")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.static_mesh_import_data.set_editor_property("combine_meshes", True)
        task.set_editor_property("options", options)
        asset_tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"Imported blaster: {f}")

# Import select car models
car_dir = os.path.join(base, "car_kit/Models/FBX format")
cars_to_import = ["ambulance.fbx", "suv.fbx", "taxi.fbx", "suv-luxury.fbx",
                   "hatchback-sports.fbx", "garbage-truck.fbx", "police.fbx", "sedan.fbx"]
for f in cars_to_import:
    fp = os.path.join(car_dir, f)
    if os.path.exists(fp):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", fp)
        task.set_editor_property("destination_path", "/Game/AnalogStrike/ImportedAssets/Cars")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.static_mesh_import_data.set_editor_property("combine_meshes", True)
        task.set_editor_property("options", options)
        asset_tools.import_asset_tasks([task])
        count += 1
        unreal.log(f"Imported car: {f}")

# Import space kit items
space_dir = os.path.join(base, "space_kit/Models/FBX format")
if os.path.exists(space_dir):
    space_files = [f for f in os.listdir(space_dir) if f.endswith(".fbx")][:15]
    for f in space_files:
        fp = os.path.join(space_dir, f)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", fp)
        task.set_editor_property("destination_path", "/Game/AnalogStrike/ImportedAssets/SpaceKit")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        task.set_editor_property("options", options)
        asset_tools.import_asset_tasks([task])
        count += 1

# Place cars around the map as environmental detail
car_dest = "/Game/AnalogStrike/ImportedAssets/Cars"
if unreal.EditorAssetLibrary.does_directory_exist(car_dest):
    car_assets = unreal.EditorAssetLibrary.list_assets(car_dest, recursive=True)
    car_positions = [
        (1000, -500, 10), (1500, 500, 10), (-1000, -1500, 10),
        (3000, -2000, 10), (-2000, 500, 10), (4000, 1000, 10),
        (6000, -200, 10), (5000, -500, 10), (-3000, -1000, 10),
        (2000, 3000, 10), (500, -4000, 10), (-4000, 2000, 10),
    ]
    placed = 0
    for pos in car_positions:
        if placed >= len(car_assets): break
        mesh = unreal.EditorAssetLibrary.load_asset(car_assets[placed % len(car_assets)])
        if mesh and isinstance(mesh, unreal.StaticMesh):
            loc = unreal.Vector(pos[0], pos[1], pos[2])
            rot = unreal.Rotator(0, random.uniform(0, 360), 0)
            actor = editor_lib.spawn_actor_from_object(mesh, loc, rot)
            if actor:
                actor.set_actor_scale3d(unreal.Vector(80, 80, 80))
                actor.set_actor_label(f"Car_{placed}")
                placed += 1

level_sub.save_current_level()
unreal.log(f"=== Imported {count} Kenney models, placed {placed} cars ===")
