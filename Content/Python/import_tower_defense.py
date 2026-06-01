"""Import Kenney Tower Defense Kit models — walls, towers, trees, rocks.
Run in UE5 editor: py /tmp/ue_import_tower_defense.py"""
import unreal
import os
import random

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_lib = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

base = os.path.expanduser("~/Downloads/kenney_models/tower-defense-kit/extracted")
fbx_dir = None
for root, dirs, files in os.walk(base):
    if any(f.endswith('.fbx') for f in files):
        fbx_dir = root
        break

if not fbx_dir:
    unreal.log_error("No FBX directory found!")
else:
    # Import select models (walls, towers, trees, rocks)
    import_keywords = ['wall', 'tower', 'tree', 'rock', 'stone', 'gate', 'fence', 'house', 'castle']
    imported = 0
    all_fbx = [f for f in os.listdir(fbx_dir) if f.endswith('.fbx')]

    for f in all_fbx:
        # Import models matching keywords + first 30 of any type
        matches_keyword = any(kw in f.lower() for kw in import_keywords)
        if matches_keyword or imported < 30:
            fp = os.path.join(fbx_dir, f)
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", fp)
            task.set_editor_property("destination_path", "/Game/AnalogStrike/ImportedAssets/TowerDefense")
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", True)
            task.set_editor_property("save", True)
            options = unreal.FbxImportUI()
            options.set_editor_property("import_mesh", True)
            options.set_editor_property("import_as_skeletal", False)
            options.static_mesh_import_data.set_editor_property("combine_meshes", True)
            task.set_editor_property("options", options)
            asset_tools.import_asset_tasks([task])
            imported += 1
            if imported >= 50:
                break

    unreal.log(f"Imported {imported} Tower Defense models")

    # Place walls and towers around checkpoints and research station
    td_dest = "/Game/AnalogStrike/ImportedAssets/TowerDefense"
    if unreal.EditorAssetLibrary.does_directory_exist(td_dest):
        td_assets = unreal.EditorAssetLibrary.list_assets(td_dest, recursive=True)
        meshes = []
        for a in td_assets:
            m = unreal.EditorAssetLibrary.load_asset(a)
            if isinstance(m, unreal.StaticMesh):
                meshes.append(m)

        if meshes:
            placed = 0
            # Place around key locations
            locations = [
                # Checkpoint perimeter
                (3000, -3000, 0), (3200, -2800, 0), (3400, -3200, 0), (2800, -3400, 0),
                # Research station walls
                (4200, 800, 0), (4200, -800, 0), (4500, 600, 0), (4500, -600, 0),
                (4800, 400, 0), (4800, -400, 0),
                # Cave entrance
                (5000, 2800, 0), (5200, 3000, 0), (4800, 3200, 0),
                # FOB walls
                (-500, -300, 0), (-500, 300, 0), (-700, 0, 0),
                # Central valley decor
                (1000, 1500, 0), (1500, -500, 0), (-500, 1000, 0),
                (2000, 2000, 0), (-1000, -1000, 0),
            ]
            for pos in locations:
                mesh = meshes[placed % len(meshes)]
                loc = unreal.Vector(pos[0], pos[1], pos[2])
                rot = unreal.Rotator(0, random.uniform(0, 360), 0)
                actor = editor_lib.spawn_actor_from_object(mesh, loc, rot)
                if actor:
                    actor.set_actor_scale3d(unreal.Vector(100, 100, 100))
                    actor.set_actor_label(f"TDKit_{placed}")
                    placed += 1
            unreal.log(f"Placed {placed} Tower Defense props")

    # Also place heal stations
    heal_class = unreal.load_class(None, "/Script/AnalogStrike.ASHealStation")
    if heal_class:
        heal_positions = [(-100, 100, 10), (2500, -2000, 10), (4500, 500, 10), (6000, 200, 10)]
        for pos in heal_positions:
            actor = editor_lib.spawn_actor_from_class(heal_class, unreal.Vector(*pos), unreal.Rotator(0,0,0))
            if actor:
                actor.set_actor_label("HealStation")

    # Place enemy spawners
    spawner_class = unreal.load_class(None, "/Script/AnalogStrike.ASEnemySpawner")
    if spawner_class:
        spawner_positions = [
            (3000, -3000, 10), (5000, 3000, 10), (-3000, 2000, 10),
            (4500, 0, 10), (0, -5000, 10),
        ]
        for pos in spawner_positions:
            actor = editor_lib.spawn_actor_from_class(spawner_class, unreal.Vector(*pos), unreal.Rotator(0,0,0))
            if actor:
                actor.set_actor_label("EnemySpawner")

    level_sub.save_current_level()
    unreal.log("=== TOWER DEFENSE IMPORT COMPLETE ===")
