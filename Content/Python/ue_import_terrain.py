"""
ANALOG STRIKE — TERRAIN IMPORT
Imports the procedural terrain mesh (~/Downloads/as_terrain.obj) as a walkable
static mesh with complex-as-simple collision, applies a green terrain material,
and places it at the world origin.

Run inside the UE5 editor:  py /tmp/ue_import_terrain.py
"""
import unreal
import os

OBJ = os.path.expanduser("~/Downloads/as_terrain.obj")
DEST = "/Game/AnalogStrike/Terrain"
editor = unreal.EditorLevelLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

if not os.path.exists(OBJ):
    unreal.log_error(f"Terrain OBJ not found: {OBJ}  (run gen_terrain.py first)")
else:
    # ── Import the OBJ as a static mesh ──
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", OBJ)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    asset_tools.import_asset_tasks([task])

    # Locate the imported mesh
    terrain = None
    for a in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
        m = unreal.EditorAssetLibrary.load_asset(a)
        if isinstance(m, unreal.StaticMesh):
            terrain = m
            break

    if not terrain:
        unreal.log_error("Terrain mesh failed to import!")
    else:
        # ── Walkable collision: use the render triangles as collision ──
        body = terrain.get_editor_property("body_setup")
        if body:
            body.set_editor_property("collision_trace_flag",
                                     unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.EditorAssetLibrary.save_asset(terrain.get_path_name())

        # ── Green terrain material ──
        mat_path = "/Game/AnalogStrike/Materials/M_Terrain"
        mat = unreal.EditorAssetLibrary.load_asset(mat_path)
        if not mat:
            mat = asset_tools.create_asset("M_Terrain", "/Game/AnalogStrike/Materials",
                                           unreal.Material, unreal.MaterialFactoryNew())
            # Base color: mossy green
            col = unreal.MaterialEditingLibrary.create_material_expression(
                mat, unreal.MaterialExpressionConstant3Vector)
            col.set_editor_property("constant", unreal.LinearColor(0.12, 0.22, 0.10, 1.0))
            unreal.MaterialEditingLibrary.connect_material_property(
                col, "", unreal.MaterialProperty.MP_BASE_COLOR)
            rough = unreal.MaterialEditingLibrary.create_material_expression(
                mat, unreal.MaterialExpressionConstant)
            rough.set_editor_property("r", 0.85)
            unreal.MaterialEditingLibrary.connect_material_property(
                rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
            unreal.MaterialEditingLibrary.recompile_material(mat)
            unreal.EditorAssetLibrary.save_asset(mat_path)
        if mat:
            terrain.set_material(0, mat)
            unreal.EditorAssetLibrary.save_asset(terrain.get_path_name())

        # ── Remove any previous terrain instance, then place at origin ──
        for a in editor.get_all_level_actors():
            if a.get_actor_label() == "AS_Terrain":
                editor.destroy_actor(a)
        actor = editor.spawn_actor_from_object(terrain, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        if actor:
            actor.set_actor_label("AS_Terrain")
            # Static so it doesn't move
            actor.set_actor_scale3d(unreal.Vector(1, 1, 1))

        level_sub.save_current_level()
        bounds = terrain.get_bounds()
        unreal.log("══════════════════════════════════════════")
        unreal.log("TERRAIN IMPORTED & PLACED at origin")
        unreal.log(f"  collision: complex-as-simple (walkable)")
        unreal.log(f"  material:  M_Terrain (green)")
        unreal.log("Run build_map next so buildings sit on the surface.")
        unreal.log("══════════════════════════════════════════")
