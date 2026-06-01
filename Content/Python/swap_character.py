"""Swap the player character model from Manny to Quinn with shooter animations.
Run this inside UE5 editor: py /tmp/ue_swap_character.py"""
import unreal

# Load the BP_ThirdPersonCharacter blueprint
bp_path = "/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"
bp = unreal.EditorAssetLibrary.load_asset(bp_path)

if not bp:
    unreal.log_error("Could not load BP_ThirdPersonCharacter!")
else:
    unreal.log(f"Loaded: {bp.get_name()}")

    # Get the CDO (Class Default Object) to modify defaults
    cdo = unreal.get_default_object(bp.generated_class())
    if cdo:
        mesh = cdo.get_editor_property("mesh")
        if mesh:
            # Try to find Quinn mesh
            quinn_paths = [
                "/Game/Characters/Mannequins/Meshes/SKM_Quinn",
                "/Game/Characters/Meshes/SKM_Quinn_Simple",
            ]
            quinn_mesh = None
            for p in quinn_paths:
                quinn_mesh = unreal.load_asset(p)
                if quinn_mesh:
                    break

            if quinn_mesh:
                mesh.set_editor_property("skeletal_mesh", quinn_mesh)
                unreal.log(f"Set mesh to: {quinn_mesh.get_name()}")
            else:
                unreal.log_warning("Quinn mesh not found, trying SK_Mannequin paths...")

            # Try to set rifle animation blueprint
            abp_paths = [
                "/Game/Characters/Anims/Shooter/ABP_TP_Rifle",
                "/Game/Variant_Shooter/Anims/ABP_TP_Rifle",
            ]
            for abp_path in abp_paths:
                abp = unreal.load_asset(abp_path)
                if abp:
                    mesh.set_editor_property("anim_class", abp.generated_class())
                    unreal.log(f"Set animation BP to: {abp.get_name()}")
                    break

    # Save the blueprint
    unreal.EditorAssetLibrary.save_asset(bp_path)
    unreal.log("=== Character model swapped! Restart PIE to see changes. ===")
