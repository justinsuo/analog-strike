"""
Analog Strike — IN-EDITOR SNAPSHOT
Run via tools/claude/snapshot.sh (which launches UE5 headless and invokes this).
Writes:
  claude/snapshot/state.json              level state + actor inventory
  claude/snapshot/raw_shots/*.png         6 screenshots (lit + flat passes)
  claude/snapshot/log_tail.txt            tail of the most recent UE log
The shell wrapper post-processes the raw screenshots into claude/snapshot/screenshots/.
"""
import unreal, os, json, datetime, glob, shutil

editor = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")
world = editor.get_editor_world()

PROJ = unreal.SystemLibrary.get_project_directory().rstrip("/")
SNAP = os.path.join(PROJ, "claude", "snapshot")
RAW  = os.path.join(SNAP, "raw_shots")
os.makedirs(RAW, exist_ok=True)

# ─── Ensure lighting so lit screenshots aren't black ───
def find(cls_name):
    return [a for a in editor.get_all_level_actors() if a.get_class().get_name() == cls_name]
def ensure_lighting():
    if not find("DirectionalLight"):
        dl = editor.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,20000), unreal.Rotator(-46,45,0))
        dlc = dl.get_component_by_class(unreal.DirectionalLightComponent)
        if dlc:
            dlc.set_intensity(8.0)
            try: dlc.set_editor_property("atmosphere_sun_light", True)
            except Exception: pass
        dl.set_actor_label("Sun")
    if not find("SkyAtmosphere"):
        editor.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0,0,0), unreal.Rotator(0,0,0)).set_actor_label("SkyAtmo")
    if not find("SkyLight"):
        sk = editor.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,16000), unreal.Rotator(0,0,0))
        skc = sk.get_component_by_class(unreal.SkyLightComponent)
        if skc:
            try: skc.set_editor_property("real_time_capture", False)
            except Exception: pass
            skc.set_intensity(1.5)
            try: skc.recapture_sky()
            except Exception: pass
        sk.set_actor_label("SkyLight")
ensure_lighting()
level_sub.save_current_level()

# ─── Dump level state to JSON ───
state = {
    "timestamp": datetime.datetime.now().isoformat(timespec="seconds"),
    "level": "OpenWorld",
    "actor_summary": {},
    "actors": [],
    "lighting": {},
    "stats": {},
}
all_actors = editor.get_all_level_actors()
for a in all_actors:
    cls = a.get_class().get_name()
    state["actor_summary"][cls] = state["actor_summary"].get(cls, 0) + 1
    loc = a.get_actor_location()
    label = a.get_actor_label()
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    mesh_name = None
    if smc:
        sm = smc.get_static_mesh()
        if sm: mesh_name = sm.get_name()
    state["actors"].append({
        "label": label, "class": cls,
        "x": round(loc.x,1), "y": round(loc.y,1), "z": round(loc.z,1),
        "mesh": mesh_name,
    })

# Lighting summary
for cls_name in ("DirectionalLight","SkyLight","SkyAtmosphere","ExponentialHeightFog"):
    for a in find(cls_name):
        d = {"position": [round(a.get_actor_location().x,0), round(a.get_actor_location().y,0), round(a.get_actor_location().z,0)]}
        for comp_cls in (unreal.DirectionalLightComponent, unreal.SkyLightComponent):
            c = a.get_component_by_class(comp_cls)
            if c:
                try: d["intensity"] = float(c.get_unbuilt_light_intensity()) if hasattr(c, "get_unbuilt_light_intensity") else float(getattr(c, "intensity", 0))
                except Exception: pass
        state["lighting"].setdefault(cls_name, []).append(d)

state["stats"]["total_actors"] = len(all_actors)
state["stats"]["terrain_present"] = any(a.get_actor_label()=="AS_Terrain" for a in all_actors)

with open(os.path.join(SNAP,"state.json"),"w") as f:
    json.dump(state, f, indent=2)
unreal.log(f"state.json written: {len(all_actors)} actors")

# ─── Render screenshots ───
RT_W, RT_H = 1600, 900
rt = unreal.RenderingLibrary.create_render_target2d(world, RT_W, RT_H, unreal.TextureRenderTargetFormat.RTF_RGBA8)
cap_actor = editor.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
cap = cap_actor.capture_component2d
cap.texture_target = rt
cap.fov_angle = 90.0
# Manual exposure to avoid washing to black
try:
    pp = cap.post_process_settings
    pp.set_editor_property("auto_exposure_min_brightness", 1.0)
    pp.set_editor_property("auto_exposure_max_brightness", 1.0)
    pp.set_editor_property("auto_exposure_bias", 1.0)
    cap.post_process_blend_weight = 1.0
except Exception as e:
    unreal.log_warning(f"post_process: {e}")

views = [
    ("overhead", unreal.Vector(0, 0, 30000),           unreal.Rotator(-89, 0, 0)),
    ("aerial",   unreal.Vector(-17000, -17000, 15000), unreal.Rotator(-28, 45, 0)),
    ("ground",   unreal.Vector(0, -4800, 300),         unreal.Rotator(-4, 90, 0)),
]
def shoot(suffix, source):
    cap.capture_source = source
    for (name, loc, rot) in views:
        cap_actor.set_actor_location_and_rotation(loc, rot, False, False)
        cap.capture_scene()
        ok = unreal.RenderingLibrary.export_render_target(world, rt, RAW, f"{name}_{suffix}.png")
        unreal.log(f"{'OK' if ok else 'FAIL'}: {name}_{suffix}.png")
shoot("lit",  unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
shoot("flat", unreal.SceneCaptureSource.SCS_BASE_COLOR)
editor.destroy_actor(cap_actor)

# ─── Copy the latest editor log tail ───
log_dir = os.path.join(PROJ, "Saved", "Logs")
logs = sorted(glob.glob(os.path.join(log_dir, "*.log")), key=os.path.getmtime, reverse=True)
if logs:
    try:
        with open(logs[0], "r", errors="ignore") as f:
            lines = f.readlines()[-300:]
        with open(os.path.join(SNAP, "log_tail.txt"), "w") as out:
            out.writelines(lines)
        unreal.log(f"log_tail.txt: {len(lines)} lines from {os.path.basename(logs[0])}")
    except Exception as e:
        unreal.log_warning(f"log copy: {e}")

unreal.log("===== SNAPSHOT.PY DONE =====")
