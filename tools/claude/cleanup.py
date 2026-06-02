"""
Cleanup: delete PIE-spawned cruft (enemies, drones, pickups) and dedupe duplicate
lighting actors. These accumulate from Play-In-Editor sessions and tank FPS.
"""
import unreal

editor = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

# Classes to fully purge — wave-spawned AI / pickup drops, never manually placed
PURGE_CLASSES = {
    "ASSecurityFrame", "ASScoutDrone", "ASEnemyBase",
    "ASSniper", "ASWarden", "ASBuilderUnit",
    "ASPickup",
    "PointLight",  # 144 orphan glow lights left over from prior PIE runs
}
# Dedupe: keep only one of each
DEDUPE_CLASSES = {"DirectionalLight", "SkyLight", "SkyAtmosphere", "ExponentialHeightFog"}

actors = editor.get_all_level_actors()
counts_before = {}
for a in actors:
    c = a.get_class().get_name()
    counts_before[c] = counts_before.get(c, 0) + 1

purged = 0
deduped = 0
seen = {}
for a in actors:
    cls = a.get_class().get_name()
    if cls in PURGE_CLASSES:
        editor.destroy_actor(a); purged += 1
    elif cls in DEDUPE_CLASSES:
        if cls in seen:
            editor.destroy_actor(a); deduped += 1
        else:
            seen[cls] = a

# Also purge any actor whose label starts with "DroppedLoot" / "BonusDrop"
for a in editor.get_all_level_actors():
    lbl = (a.get_actor_label() or "").lower()
    if lbl.startswith("droppedloot") or lbl.startswith("bonusdrop"):
        editor.destroy_actor(a); purged += 1

# Purge ORPHAN StaticMeshActors using engine basic shapes (Cube/Cone/Sphere/Cylinder).
# These are scenery cruft from pre-Kenney-kit iterations — thousands of them.
# Don't touch game-class actors (ASTurret etc.) that legitimately use cubes.
ENGINE_BASIC = {"Cube", "Cone", "Sphere", "Cylinder"}
orphan_basic = 0
for a in editor.get_all_level_actors():
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    smc = a.get_component_by_class(unreal.StaticMeshComponent)
    if not smc: continue
    try:
        sm = smc.get_editor_property("static_mesh")
    except Exception:
        sm = None
    if sm and sm.get_name() in ENGINE_BASIC:
        editor.destroy_actor(a); orphan_basic += 1
unreal.log(f"Purged {orphan_basic} orphan basic-shape StaticMeshActors")
purged += orphan_basic

# Dedupe actors at identical (class, position) — kill duplicate Relays, NPCs, doors etc.
from collections import defaultdict
sig_map = defaultdict(list)
for a in editor.get_all_level_actors():
    cls = a.get_class().get_name()
    # Skip the bulk static mesh scenery (would falsely dedupe stacked crates etc.)
    if cls == "StaticMeshActor": continue
    loc = a.get_actor_location()
    key = (cls, round(loc.x), round(loc.y), round(loc.z))
    sig_map[key].append(a)
pos_deduped = 0
for key, lst in sig_map.items():
    for extra in lst[1:]:
        editor.destroy_actor(extra); pos_deduped += 1
unreal.log(f"Position-deduped {pos_deduped} duplicate actors")
deduped += pos_deduped

unreal.log(f"Purged {purged} runtime-spawned actors")
unreal.log(f"Deduped {deduped} duplicate lighting actors")
unreal.log(f"Before/after totals:")
after = {}
for a in editor.get_all_level_actors():
    c = a.get_class().get_name()
    after[c] = after.get(c, 0) + 1
for cls in sorted(set(list(counts_before.keys()) + list(after.keys()))):
    b, n = counts_before.get(cls, 0), after.get(cls, 0)
    if b != n: unreal.log(f"  {cls}: {b} -> {n}")

level_sub.save_current_level()
unreal.log("===== CLEANUP DONE =====")
