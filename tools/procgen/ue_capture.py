"""
ANALOG STRIKE — SCENE CAPTURE TO PNG (lit, with fallback unlit pass)
Ensures the level has lighting, then renders the open world from several vantage
points to 8-bit PNGs in /tmp/analogstrike_shots/.
Run inside the UE5 editor:  py /tmp/ue_capture.py
"""
import unreal, os

OUT_DIR = "/tmp/analogstrike_shots"
os.makedirs(OUT_DIR, exist_ok=True)
editor = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")
world = editor.get_editor_world()

# ── Ensure lighting so captures aren't black ──
def ensure_lighting():
    try:
        existing = {a.get_class().get_name() for a in editor.get_all_level_actors()}
        if "DirectionalLight" not in existing:
            dl = editor.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,20000), unreal.Rotator(-46,45,0))
            dlc = dl.get_component_by_class(unreal.DirectionalLightComponent)
            if dlc:
                dlc.set_intensity(6.0)
                try: dlc.set_editor_property("atmosphere_sun_light", True)
                except Exception: pass
            dl.set_actor_label("Sun")
        if "SkyAtmosphere" not in existing:
            editor.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0,0,0), unreal.Rotator(0,0,0)).set_actor_label("SkyAtmo")
        if "SkyLight" not in existing:
            sk = editor.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,16000), unreal.Rotator(0,0,0))
            skc = sk.get_component_by_class(unreal.SkyLightComponent)
            if skc:
                try: skc.set_editor_property("real_time_capture", True)
                except Exception: pass
                skc.set_intensity(1.5)
            sk.set_actor_label("SkyLight")
        if "ExponentialHeightFog" not in existing:
            editor.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0,0,500), unreal.Rotator(0,0,0)).set_actor_label("Fog")
        unreal.log("Lighting ensured")
    except Exception as e:
        unreal.log_warning(f"Lighting setup issue: {e}")
ensure_lighting()
level_sub.save_current_level()

# ── Render target (8-bit -> real PNG) ──
RT_W, RT_H = 1600, 900
rt = unreal.RenderingLibrary.create_render_target2d(world, RT_W, RT_H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap_actor = editor.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
cap = cap_actor.capture_component2d
cap.texture_target = rt
cap.fov_angle = 90.0

views = [
    ("as_overhead", unreal.Vector(0, 0, 30000),           unreal.Rotator(-89, 0, 0)),
    ("as_aerial",   unreal.Vector(-17000, -17000, 15000), unreal.Rotator(-28, 45, 0)),
    ("as_mainbase", unreal.Vector(0, -3500, 3500),        unreal.Rotator(-25, 90, 0)),
    ("as_lake",     unreal.Vector(9500, -15000, 3500),    unreal.Rotator(-18, 90, 0)),
    ("as_ground",   unreal.Vector(0, -4800, 300),         unreal.Rotator(-4, 90, 0)),
]

def shoot(suffix, source):
    cap.capture_source = source
    n = 0
    for (name, loc, rot) in views:
        cap_actor.set_actor_location_and_rotation(loc, rot, False, False)
        cap.capture_scene()
        ok = unreal.RenderingLibrary.export_render_target(world, rt, OUT_DIR, name + suffix + ".png")
        unreal.log(f"{'Saved' if ok else 'FAILED'}: {name}{suffix}.png")
        n += 1
    return n

# Lit pass (looks like the game) + guaranteed unlit base-color pass
shoot("", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
shoot("_flat", unreal.SceneCaptureSource.SCS_BASE_COLOR)

editor.destroy_actor(cap_actor)
unreal.log("===== CAPTURE COMPLETE =====")
