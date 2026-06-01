"""Customize the player character appearance — dark tactical operative.
Creates custom materials and applies them. Run inside UE5 editor."""
import unreal

# Create a custom material instance for the player
base_mat_path = "/Game/Characters/Materials/M_Mannequin"
base_mat = unreal.EditorAssetLibrary.load_asset(base_mat_path)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

if base_mat:
    # Create tactical body material (dark with blue-green accent lines)
    body_mi = asset_tools.create_asset(
        "MI_Player_Body", "/Game/AnalogStrike/Materials",
        unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if body_mi:
        body_mi.set_editor_property("parent", base_mat)
        # Dark body
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            body_mi, "BaseColor", unreal.LinearColor(0.03, 0.04, 0.06, 1))
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            body_mi, "Roughness", 0.5)
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            body_mi, "Metallic", 0.4)
        unreal.EditorAssetLibrary.save_asset("/Game/AnalogStrike/Materials/MI_Player_Body")
        unreal.log("Created MI_Player_Body (dark tactical)")

    # Create accent material (glowing cyan lines for the "augmented operative" look)
    accent_mi = asset_tools.create_asset(
        "MI_Player_Accent", "/Game/AnalogStrike/Materials",
        unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if accent_mi:
        accent_mi.set_editor_property("parent", base_mat)
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            accent_mi, "BaseColor", unreal.LinearColor(0.05, 0.3, 0.4, 1))
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            accent_mi, "EmissiveColor", unreal.LinearColor(0.1, 0.8, 1.0, 1))
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            accent_mi, "Roughness", 0.2)
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            accent_mi, "Metallic", 0.8)
        unreal.EditorAssetLibrary.save_asset("/Game/AnalogStrike/Materials/MI_Player_Accent")
        unreal.log("Created MI_Player_Accent (cyan glow)")

    # Now try to apply to the character blueprint
    bp = unreal.EditorAssetLibrary.load_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
    if bp:
        cdo = unreal.get_default_object(bp.generated_class())
        if cdo:
            mesh_comp = cdo.get_editor_property("mesh")
            if mesh_comp:
                # Apply body material to slot 0, accent to slot 1
                body_mat = unreal.EditorAssetLibrary.load_asset("/Game/AnalogStrike/Materials/MI_Player_Body")
                accent_mat = unreal.EditorAssetLibrary.load_asset("/Game/AnalogStrike/Materials/MI_Player_Accent")
                if body_mat:
                    mesh_comp.set_material(0, body_mat)
                    unreal.log("Applied body material to player")
                if accent_mat and mesh_comp.get_num_materials() > 1:
                    mesh_comp.set_material(1, accent_mat)
                    unreal.log("Applied accent material to player")

        unreal.EditorAssetLibrary.save_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
        unreal.log("Saved BP_ThirdPersonCharacter with new materials")

    # Also try to set Quinn mesh
    quinn_paths = [
        "/Game/Characters/Mannequins/Meshes/SKM_Quinn",
        "/Game/Characters/Meshes/SKM_Quinn_Simple",
    ]
    for qp in quinn_paths:
        qm = unreal.EditorAssetLibrary.load_asset(qp)
        if qm:
            if bp:
                cdo = unreal.get_default_object(bp.generated_class())
                if cdo:
                    mesh_comp = cdo.get_editor_property("mesh")
                    if mesh_comp:
                        mesh_comp.set_editor_property("skeletal_mesh_asset", qm)
                        unreal.log(f"Set character mesh to {qm.get_name()}")
                        break

    # Set the rifle animation blueprint
    abp_paths = [
        "/Game/Characters/Anims/Shooter/ABP_TP_Rifle",
        "/Game/Variant_Shooter/Anims/ABP_TP_Rifle",
    ]
    for ap in abp_paths:
        abp = unreal.EditorAssetLibrary.load_asset(ap)
        if abp:
            if bp:
                cdo = unreal.get_default_object(bp.generated_class())
                if cdo:
                    mesh_comp = cdo.get_editor_property("mesh")
                    if mesh_comp:
                        mesh_comp.set_editor_property("anim_class", abp.generated_class())
                        unreal.log(f"Set rifle animation BP: {abp.get_name()}")
                        break

    unreal.EditorAssetLibrary.save_asset("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")

else:
    unreal.log_warning("Base mannequin material not found, trying alternate paths...")
    # Try finding any material to use as base
    alt_mats = [
        "/Game/Characters/Materials/Manny/MI_Manny_01_New",
        "/Game/Characters/Materials/Quinn/MI_Quinn_01",
    ]
    for am in alt_mats:
        mat = unreal.EditorAssetLibrary.load_asset(am)
        if mat:
            unreal.log(f"Found alternate material: {am}")
            break

unreal.log("=== PLAYER CHARACTER CUSTOMIZATION COMPLETE ===")
unreal.log("Restart PIE to see the new dark tactical operative look!")
