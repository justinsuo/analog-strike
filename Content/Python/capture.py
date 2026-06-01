"""
ANALOG STRIKE — SCENE CAPTURE TO PNG
Uses a SceneCapture2D + render target to synchronously render the level from
several vantage points and export PNGs to /tmp/analogstrike_shots/.
This does NOT use the editor viewport and does NOT grab your screen — it renders
straight from the level to image files that can be reviewed.

Run inside the UE5 editor:  py /tmp/ue_capture.py
"""
import unreal
import os

OUT_DIR = "/tmp/analogstrike_shots"
os.makedirs(OUT_DIR, exist_ok=True)

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

editor = unreal.EditorLevelLibrary
world = unreal.EditorLevelLibrary.get_editor_world()

# Create a render target to draw into
RT_W, RT_H = 1600, 900
render_target = unreal.RenderingLibrary.create_render_target2d(world, RT_W, RT_H)

# Spawn one SceneCapture2D we reposition for each shot
cap_actor = editor.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
cap = cap_actor.capture_component2d
cap.texture_target = render_target
cap.capture_source = unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR  # looks like the game
cap.fov_angle = 90.0
cap.show_flags.set_editor_property("temporal_aa", True) if hasattr(cap, "show_flags") else None

# (filename, location, rotation[pitch, yaw, roll])
views = [
    ("as_overhead", unreal.Vector(1000, 500, 16000), unreal.Rotator(-89, 0, 0)),   # whole map top-down
    ("as_command",  unreal.Vector(5500, 1400, 2600), unreal.Rotator(-32, 90, 0)),  # command center
    ("as_spawn",    unreal.Vector(-3700, 0, 1800),   unreal.Rotator(-22, 0, 0)),   # barracks / spawn
    ("as_ground",   unreal.Vector(-200, 0, 200),     unreal.Rotator(-3, 30, 0)),   # player eye-level
    ("as_east",     unreal.Vector(2800, 0, 2400),    unreal.Rotator(-30, 0, 0)),   # warehouses / east
]

count = 0
for (name, loc, rot) in views:
    cap_actor.set_actor_location_and_rotation(loc, rot, False, False)
    cap.capture_scene()  # synchronous render into the render target
    ok = unreal.RenderingLibrary.export_render_target(world, render_target, OUT_DIR, name + ".png")
    unreal.log(f"{'Saved' if ok else 'FAILED'}: {OUT_DIR}/{name}.png")
    count += 1

# Clean up the capture actor
editor.destroy_actor(cap_actor)

unreal.log("══════════════════════════════════════════")
unreal.log(f"CAPTURE COMPLETE — {count} images in {OUT_DIR}")
unreal.log("══════════════════════════════════════════")
