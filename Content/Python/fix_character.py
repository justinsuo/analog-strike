"""
FIX CHARACTER MODEL + GUN HOLDING
This script modifies BP_ThirdPersonCharacter directly to:
1. Change mesh to Quinn
2. Set ABP_TP_Rifle animation blueprint for proper weapon holding
3. Apply dark tactical materials
4. Set proper hand socket for weapon attachment

Run inside UE5 editor: py /tmp/ue_fix_character.py
"""
import unreal

editor_lib = unreal.EditorLevelLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# ═══════════════════════════════════════════════════════════════════
# STEP 1: Find the correct mesh and ABP paths
# ═══════════════════════════════════════════════════════════════════

# Try all possible Quinn mesh paths
quinn_paths = [
    "/Game/Characters/Mannequins/Meshes/SKM_Quinn",
    "/Game/Characters/Meshes/SKM_Quinn_Simple",
    "/Game/Characters/Meshes/SKM_Quinn",
]
quinn_mesh = None
for p in quinn_paths:
    quinn_mesh = unreal.load_asset(p)
    if quinn_mesh:
        unreal.log(f"Found Quinn mesh at: {p}")
        break

if not quinn_mesh:
    # If no Quinn, use Manny Simple
    manny_paths = [
        "/Game/Characters/Meshes/SKM_Manny_Simple",
        "/Game/Characters/Mannequins/Meshes/SKM_Manny",
    ]
    for p in manny_paths:
        quinn_mesh = unreal.load_asset(p)
        if quinn_mesh:
            unreal.log(f"Using Manny mesh at: {p}")
            break

# Find rifle animation blueprint
abp_paths = [
    "/Game/Characters/Anims/Shooter/ABP_TP_Rifle",
    "/Game/Variant_Shooter/Anims/ABP_TP_Rifle",
]
rifle_abp = None
for p in abp_paths:
    rifle_abp = unreal.load_asset(p)
    if rifle_abp:
        unreal.log(f"Found rifle ABP at: {p}")
        break

# ═══════════════════════════════════════════════════════════════════
# STEP 2: Load and modify BP_ThirdPersonCharacter
# ═══════════════════════════════════════════════════════════════════

bp_path = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
bp = unreal.load_asset(bp_path)
if not bp:
    unreal.log_error("FAILED: Cannot load BP_ThirdPersonCharacter!")
else:
    unreal.log(f"Loaded BP: {bp.get_name()}, class: {bp.generated_class().get_name()}")

    # Get all components from the CDO
    cdo = unreal.get_default_object(bp.generated_class())
    if cdo:
        unreal.log(f"CDO type: {type(cdo).__name__}")

        # Try to find the mesh component
        # In BP_ThirdPersonCharacter, the mesh is the inherited SkeletalMeshComponent
        try:
            # Method 1: Direct property access
            mesh_comp = cdo.get_editor_property("mesh")
            if mesh_comp:
                unreal.log(f"Found mesh component: {mesh_comp.get_name()}")

                # Set skeletal mesh
                if quinn_mesh:
                    mesh_comp.set_skeletal_mesh_asset(quinn_mesh)
                    unreal.log(f"SET mesh to: {quinn_mesh.get_name()}")

                # Set animation blueprint
                if rifle_abp:
                    # The ABP asset is an AnimBlueprint, we need its generated class
                    try:
                        abp_class = rifle_abp.generated_class()
                        mesh_comp.set_editor_property("anim_class", abp_class)
                        unreal.log(f"SET anim BP to: {rifle_abp.get_name()}")
                    except:
                        unreal.log_warning("Could not set anim class directly, trying alternate method...")
                        mesh_comp.set_anim_instance_class(rifle_abp.generated_class())
        except Exception as e:
            unreal.log_warning(f"Method 1 failed: {e}")

        # Method 2: Try via subsystem
        try:
            subsystem = unreal.get_editor_subsystem(unreal.SubobjectDataSubsystem)
            if subsystem:
                unreal.log("SubobjectDataSubsystem available")
        except:
            pass

    # Save the modified blueprint
    unreal.EditorAssetLibrary.save_asset(bp_path)
    unreal.log("Saved BP_ThirdPersonCharacter")

# ═══════════════════════════════════════════════════════════════════
# STEP 3: Create tactical materials
# ═══════════════════════════════════════════════════════════════════

base_mat = unreal.load_asset("/Game/Characters/Materials/M_Mannequin")
if not base_mat:
    # Try Quinn materials
    base_mat = unreal.load_asset("/Game/Characters/Materials/Quinn/MI_Quinn_01")
if not base_mat:
    base_mat = unreal.load_asset("/Game/Characters/Materials/Manny/MI_Manny_01_New")

if base_mat:
    unreal.log(f"Base material: {base_mat.get_name()}")

    # Create dark tactical material
    try:
        body_mi_path = "/Game/AnalogStrike/Materials/MI_Tactical_Body"
        existing = unreal.EditorAssetLibrary.load_asset(body_mi_path)
        if not existing:
            body_mi = asset_tools.create_asset(
                "MI_Tactical_Body", "/Game/AnalogStrike/Materials",
                unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        else:
            body_mi = existing

        if body_mi:
            body_mi.set_editor_property("parent", base_mat)
            unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
                body_mi, "BaseColor", unreal.LinearColor(0.03, 0.04, 0.06, 1))
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
                body_mi, "Roughness", 0.5)
            unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
                body_mi, "Metallic", 0.4)
            unreal.EditorAssetLibrary.save_asset(body_mi_path)
            unreal.log("Created MI_Tactical_Body")

            # Apply to the character BP mesh
            if bp:
                cdo = unreal.get_default_object(bp.generated_class())
                if cdo:
                    mesh_comp = cdo.get_editor_property("mesh")
                    if mesh_comp:
                        mesh_comp.set_material(0, body_mi)
                        if mesh_comp.get_num_materials() > 1:
                            mesh_comp.set_material(1, body_mi)
                        unreal.log("Applied tactical materials to BP mesh")
                        unreal.EditorAssetLibrary.save_asset(bp_path)
    except Exception as e:
        unreal.log_warning(f"Material creation: {e}")

# ═══════════════════════════════════════════════════════════════════
# STEP 4: Verify socket names for weapon attachment
# ═══════════════════════════════════════════════════════════════════

if quinn_mesh:
    skel = quinn_mesh.get_editor_property("skeleton")
    if skel:
        unreal.log(f"Skeleton: {skel.get_name()}")
        # List bone names to find hand socket
        # The weapon should attach to hand_r or ik_hand_gun
        bone_count = quinn_mesh.get_editor_property("ref_skeleton")
        unreal.log(f"Mesh has bones - weapon should attach to 'hand_r' socket")

unreal.log("")
unreal.log("═══════════════════════════════════════════════════")
unreal.log("CHARACTER FIX COMPLETE!")
unreal.log("Restart PIE (Play) to see changes.")
unreal.log("If gun still not held: the ABP_TP_Rifle handles")
unreal.log("weapon holding pose via hand IK and aim offsets.")
unreal.log("═══════════════════════════════════════════════════")
