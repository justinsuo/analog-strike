"""Import FBX assets only (no actor placement - that needs the editor open)."""
import unreal
import os

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
downloads = "/Users/justinsuo/Downloads"

imports = [
    (os.path.join(downloads, "evironment-pack-egypt-map/source/Final_S01.fbx"), "/Game/AnalogStrike/ImportedAssets/EgyptBuildings"),
    (os.path.join(downloads, "free-military-drone-full-pack-in-description/source/QuadDrone [mesh].fbx"), "/Game/AnalogStrike/ImportedAssets/MilitaryDrone"),
    (os.path.join(downloads, "cinematic-plains-and-mountains/source/Mesher.fbx"), "/Game/AnalogStrike/ImportedAssets/Mountains"),
    (os.path.join(downloads, "source/Sword_low.fbx"), "/Game/AnalogStrike/ImportedAssets/Sword"),
    (os.path.join(downloads, "source/axeRune_low.fbx"), "/Game/AnalogStrike/ImportedAssets/Axe"),
    (os.path.join(downloads, "sm_rifle.fbx"), "/Game/AnalogStrike/ImportedAssets/Rifle"),
    ("/tmp/granite_stone/Stone5ok17.FBX", "/Game/AnalogStrike/ImportedAssets/GraniteStone"),
    (os.path.join(downloads, "Metal_Rock_low/SM_Metal_Rock_low.fbx"), "/Game/AnalogStrike/ImportedAssets/MetalRock"),
]

count = 0
for fbx, dest in imports:
    if not os.path.exists(fbx):
        unreal.log_warning(f"Skip: {fbx}")
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("import_as_skeletal", False)
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])
    count += 1
    unreal.log(f"Imported: {os.path.basename(fbx)} -> {dest}")

# Import textures
tex_dirs = [
    (os.path.join(downloads, "evironment-pack-egypt-map/textures"), "/Game/AnalogStrike/ImportedAssets/EgyptTextures"),
    (os.path.join(downloads, "free-military-drone-full-pack-in-description/textures"), "/Game/AnalogStrike/ImportedAssets/DroneTextures"),
    (os.path.join(downloads, "cinematic-plains-and-mountains/textures"), "/Game/AnalogStrike/ImportedAssets/MountainTextures"),
    (os.path.join(downloads, "Metal_Rock_low"), "/Game/AnalogStrike/ImportedAssets/RockTextures"),
    ("/tmp/granite_stone", "/Game/AnalogStrike/ImportedAssets/StoneTextures"),
]
tex_count = 0
for tdir, tdest in tex_dirs:
    if not os.path.exists(tdir): continue
    for f in os.listdir(tdir):
        if f.lower().endswith((".png", ".jpg", ".jpeg", ".tga")):
            t = unreal.AssetImportTask()
            t.set_editor_property("filename", os.path.join(tdir, f))
            t.set_editor_property("destination_path", tdest)
            t.set_editor_property("automated", True)
            t.set_editor_property("replace_existing", True)
            t.set_editor_property("save", True)
            asset_tools.import_asset_tasks([t])
            tex_count += 1

unreal.log(f"=== IMPORT DONE: {count} meshes, {tex_count} textures ===")
