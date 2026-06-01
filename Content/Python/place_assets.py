"""Place imported assets in the OpenWorld level. Run this INSIDE UE5 editor via py ExecCmds."""
import unreal
import random

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

editor_lib = unreal.EditorLevelLibrary
placed = 0

# Map of imported meshes to their placement positions
placements = {
    "/Game/AnalogStrike/ImportedAssets/GraniteStone": {
        "scale": 3.0,
        "positions": [
            (-3000, 2000, 0), (-3500, 2500, 0), (-4000, 1500, 0),
            (2000, -3000, 0), (2500, -3500, 0), (-1000, 5000, 0),
            (4000, 3000, 0), (5500, -1000, 0), (-2500, -2000, 0),
            (1500, 4500, 0), (-5000, 3000, 0), (3500, 6000, 0),
        ]
    },
    "/Game/AnalogStrike/ImportedAssets/MetalRock": {
        "scale": 4.0,
        "positions": [
            (1000, 1000, 0), (3000, 3000, 0), (-2000, -1000, 0),
            (5000, 1000, 0), (-1000, 3000, 0), (4000, -2000, 0),
            (2000, 5000, 0), (-3000, -3000, 0), (6000, 4000, 0),
            (7000, 1000, 0), (-4000, 5000, 0), (1000, -4000, 0),
        ]
    },
    "/Game/AnalogStrike/ImportedAssets/Mountains": {
        "scale": 8.0,
        "positions": [
            (0, 18000, -1000), (-12000, 12000, -800), (12000, 12000, -800),
        ]
    },
    "/Game/AnalogStrike/ImportedAssets/EgyptBuildings": {
        "scale": 1.0,
        "positions": [
            (8000, 5000, 0), (8500, 5500, 0), (7500, 5800, 0),
            (9000, 4500, 0), (7800, 6200, 0),
        ]
    },
    "/Game/AnalogStrike/ImportedAssets/MilitaryDrone": {
        "scale": 2.0,
        "positions": [
            (3000, -2000, 300), (6000, 2000, 250), (-2000, 4000, 350),
        ]
    },
}

for folder, cfg in placements.items():
    # Find a static mesh in this folder
    assets = unreal.EditorAssetLibrary.list_assets(folder, recursive=True)
    mesh = None
    for a in assets:
        loaded = unreal.EditorAssetLibrary.load_asset(a)
        if isinstance(loaded, unreal.StaticMesh):
            mesh = loaded
            break

    if not mesh:
        unreal.log_warning(f"No mesh found in {folder}")
        continue

    for pos in cfg["positions"]:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        yaw = random.uniform(0, 360)
        rot = unreal.Rotator(0, yaw, 0)
        scale = cfg["scale"]
        if "Stone" in folder or "Rock" in folder:
            scale *= random.uniform(0.7, 1.3)

        actor = editor_lib.spawn_actor_from_object(mesh, loc, rot)
        if actor:
            actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
            actor.set_actor_label(f"Fab_{folder.split('/')[-1]}_{placed}")
            placed += 1

level_sub.save_current_level()
unreal.log(f"=== Placed {placed} assets in level ===")
