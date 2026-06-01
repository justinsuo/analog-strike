"""Import Kenney blocky character FBX files and their textures. Run inside UE5 editor."""
import unreal
import os

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
fbx_dir = os.path.expanduser("~/Downloads/kenney_blocky-characters/Models/FBX format")
tex_dir = os.path.join(fbx_dir, "Textures")
dest = "/Game/AnalogStrike/ImportedAssets/KenneyCharacters"
count = 0

# Import all 18 character FBX files
if os.path.exists(fbx_dir):
    for f in sorted(os.listdir(fbx_dir)):
        if f.endswith(".fbx"):
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", os.path.join(fbx_dir, f))
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
            unreal.log(f"Imported: {f}")

# Import textures
if os.path.exists(tex_dir):
    tex_count = 0
    for f in os.listdir(tex_dir):
        if f.lower().endswith((".png", ".jpg", ".jpeg")):
            t = unreal.AssetImportTask()
            t.set_editor_property("filename", os.path.join(tex_dir, f))
            t.set_editor_property("destination_path", dest + "/Textures")
            t.set_editor_property("automated", True)
            t.set_editor_property("replace_existing", True)
            t.set_editor_property("save", True)
            asset_tools.import_asset_tasks([t])
            tex_count += 1
    unreal.log(f"Imported {tex_count} textures")

unreal.log(f"=== Imported {count} Kenney character models ===")
