"""
EDITOR PERF + VISIBILITY FIX — open editor:
  Tools  ->  Execute Python Script...  ->  /tmp/ue_fix_now.py
Disables Lumen, virtual shadow maps, realtime sky capture, and shadow casting on
scenery; drops scalability to Low; moves the viewport to a good spot; gives the
terrain a guaranteed-visible material.
"""
import unreal

editor = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = editor.get_editor_world()
def cmd(c):
    try: unreal.SystemLibrary.execute_console_command(world, c)
    except Exception as e: unreal.log_warning(f"cmd '{c}': {e}")

# ─── 1) Console commands: kill the expensive rendering features ───
for c in [
    "r.DynamicGlobalIlluminationMethod 0",   # Lumen GI OFF
    "r.ReflectionMethod 0",                  # Lumen reflections OFF
    "r.Shadow.Virtual.Enable 0",             # Virtual shadow maps OFF
    "r.Shadow.MaxResolution 512",            # tiny shadow maps
    "r.Shadow.RadiusThreshold 0.04",         # cull small shadows
    "r.ScreenPercentage 70",                 # render at 70% res
    "sg.ShadowQuality 1",                    # low shadows
    "sg.PostProcessQuality 1",               # low post
    "sg.EffectsQuality 1",
    "sg.FoliageQuality 1",
    "sg.GlobalIlluminationQuality 0",
    "sg.ReflectionQuality 1",
    "sg.ViewDistanceQuality 1",
    "r.AntiAliasingMethod 1",                # cheap AA (FXAA)
    "r.Lumen.HardwareRayTracing 0",
    "r.Streaming.PoolSize 1500",
    "r.ForceLOD -1",
    "r.MaxAnisotropy 1",
    "r.SkeletalMeshLODBias 1",
    "r.StaticMeshLODDistanceScale 0.5",
]:
    cmd(c)
unreal.log("Console perf flags applied")

# ─── 2) Strip shadow casting from all StaticMeshComponents (massive win) ───
hidden_shadows = 0
for a in editor.get_all_level_actors():
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        try:
            smc.set_cast_shadow(False)
            hidden_shadows += 1
        except Exception:
            try:
                smc.set_editor_property("cast_shadow", False); hidden_shadows += 1
            except Exception: pass
unreal.log(f"Disabled shadows on {hidden_shadows} components")

# ─── 3) SkyLight: realtime capture OFF + one-time bake ───
for a in editor.get_all_level_actors():
    if a.get_class().get_name() == "SkyLight":
        skc = a.get_component_by_class(unreal.SkyLightComponent)
        if skc:
            try: skc.set_editor_property("real_time_capture", False)
            except Exception: pass
            try: skc.recapture_sky()
            except Exception: pass

# ─── 4) Sun: bright and tagged as the atmosphere sun ───
for a in editor.get_all_level_actors():
    if a.get_class().get_name() == "DirectionalLight":
        dlc = a.get_component_by_class(unreal.DirectionalLightComponent)
        if dlc:
            dlc.set_intensity(8.0)
            try:
                dlc.set_editor_property("atmosphere_sun_light", True)
                dlc.set_editor_property("dynamic_shadow_distance_moveable_light", 5000.0)  # short shadows
            except Exception: pass

# ─── 5) Terrain: assign a guaranteed-visible engine material ───
terrain = None
for a in editor.get_all_level_actors():
    if a.get_actor_label() == "AS_Terrain":
        terrain = a; break
if not terrain:
    for a in editor.get_all_level_actors():
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc and smc.get_static_mesh() and "as_terrain" in smc.get_static_mesh().get_name().lower():
            terrain = a; break
if terrain:
    smc = terrain.get_component_by_class(unreal.StaticMeshComponent)
    if smc:
        for p in ["/Engine/EngineMaterials/WorldGridMaterial",
                  "/Engine/BasicShapes/BasicShapeMaterial"]:
            m = unreal.load_asset(p)
            if m:
                for i in range(smc.get_num_materials()): smc.set_material(i, m)
                unreal.log(f"Terrain material -> {p}")
                break

# ─── 6) Move viewport camera to an aerial overlook ───
try:
    editor.set_level_viewport_camera_info(
        unreal.Vector(-15000, -15000, 9000), unreal.Rotator(-28, 45, 0))
except Exception as e:
    unreal.log_warning(f"viewport move: {e}")

# ─── 7) Save level ───
try: level_sub.save_current_level()
except Exception as e: unreal.log_warning(f"save: {e}")

unreal.log("==== PERF + VISIBILITY FIX APPLIED ====")
