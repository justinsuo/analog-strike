"""Spawn environmental hazards: explosive barrels, electric fences. Run in UE5 editor."""
import unreal

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

editor_lib = unreal.EditorLevelLibrary
count = 0

# Explosive barrels scattered around combat areas
barrel_class = unreal.load_class(None, "/Script/AnalogStrike.ASExplosiveBarrel")
if barrel_class:
    barrel_positions = [
        # Near checkpoint
        (3200, -3200, 10), (3100, -3100, 10), (3300, -3000, 10),
        # Research station
        (4500, 500, 10), (4200, -200, 10),
        # Cave entrance
        (5200, 3200, 10), (5500, 3500, 10),
        # Central valley combat areas
        (1500, 1500, 10), (-500, 2000, 10), (2000, -1000, 10),
        # Pine forest
        (-4500, 500, 10), (-4200, -200, 10), (-4800, 800, 10),
        # Near helipad
        (6500, 100, 10), (6200, -300, 10),
        # Bunker entrance
        (500, -5200, 10), (-200, -5500, 10),
    ]
    for pos in barrel_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        rot = unreal.Rotator(0, 0, 0)
        actor = editor_lib.spawn_actor_from_class(barrel_class, loc, rot)
        if actor:
            actor.set_actor_label(f"ExplosiveBarrel_{count}")
            count += 1

# Electric fences at facility entrances
fence_class = unreal.load_class(None, "/Script/AnalogStrike.ASElectricFence")
if fence_class:
    fence_positions = [
        # Research station perimeter
        (4000, 1000, 10, 0), (4000, -1000, 10, 0),
        (4800, 0, 10, 90),
        # Cave system entrance
        (5000, 3000, 10, 45),
        # Checkpoint barriers
        (3000, -3500, 10, 30), (3500, -3500, 10, -30),
        # Bunker perimeter
        (200, -5000, 10, 0), (-200, -5000, 10, 0),
    ]
    for pos in fence_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        rot = unreal.Rotator(0, pos[3], 0)
        actor = editor_lib.spawn_actor_from_class(fence_class, loc, rot)
        if actor:
            actor.set_actor_label(f"ElectricFence_{count}")
            count += 1

level_sub.save_current_level()
unreal.log(f"=== Spawned {count} environmental hazards ===")
