"""Spawn snipers, ammo crates, explosive barrels, electric fences. Run in UE5 editor."""
import unreal

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

editor_lib = unreal.EditorLevelLibrary
count = 0

# Snipers on elevated positions
sniper_class = unreal.load_class(None, "/Script/AnalogStrike.ASSniper")
if sniper_class:
    sniper_positions = [
        (5000, 2000, 200), (-3000, 4000, 150), (7000, -1000, 200),
        (2000, 6000, 180), (-5000, -2000, 150), (4000, -4000, 200),
    ]
    for pos in sniper_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(sniper_class, loc, unreal.Rotator(0,0,0))
        if actor:
            actor.set_actor_label(f"Sniper_{count}")
            count += 1

# Ammo crates at strategic locations
crate_class = unreal.load_class(None, "/Script/AnalogStrike.ASAmmoCrate")
if crate_class:
    crate_positions = [
        # Near spawn/FOB
        (-200, -100, 20),
        # Central valley
        (1000, 0, 20),
        # Checkpoint approach
        (2500, -2500, 20),
        # Research station
        (4500, 200, 20),
        # Cave entrance
        (5000, 3500, 20),
        # Pine forest
        (-4000, 0, 20),
        # Mountain ridge approach
        (0, 6000, 20),
        # Near helipad/extraction
        (6500, 0, 20),
    ]
    for pos in crate_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(crate_class, loc, unreal.Rotator(0,0,0))
        if actor:
            actor.set_actor_label(f"AmmoCrate_{count}")
            # Crates near extraction also refill grenades
            if pos[0] > 5000:
                actor.set_editor_property("b_refill_grenades", True)
            count += 1

# Explosive barrels
barrel_class = unreal.load_class(None, "/Script/AnalogStrike.ASExplosiveBarrel")
if barrel_class:
    barrel_positions = [
        (3200, -3200, 10), (3100, -3100, 10), (3300, -3000, 10),
        (4500, 500, 10), (4200, -200, 10),
        (5200, 3200, 10), (5500, 3500, 10),
        (1500, 1500, 10), (-500, 2000, 10), (2000, -1000, 10),
        (-4500, 500, 10), (-4200, -200, 10), (-4800, 800, 10),
        (6500, 100, 10), (6200, -300, 10),
        (500, -5200, 10), (-200, -5500, 10),
    ]
    for pos in barrel_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(barrel_class, loc, unreal.Rotator(0,0,0))
        if actor:
            actor.set_actor_label(f"ExplosiveBarrel_{count}")
            count += 1

# Electric fences
fence_class = unreal.load_class(None, "/Script/AnalogStrike.ASElectricFence")
if fence_class:
    fence_data = [
        (4000, 1000, 10, 0), (4000, -1000, 10, 0), (4800, 0, 10, 90),
        (5000, 3000, 10, 45), (3000, -3500, 10, 30), (3500, -3500, 10, -30),
        (200, -5000, 10, 0), (-200, -5000, 10, 0),
    ]
    for pos in fence_data:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        rot = unreal.Rotator(0, pos[3], 0)
        actor = editor_lib.spawn_actor_from_class(fence_class, loc, rot)
        if actor:
            actor.set_actor_label(f"ElectricFence_{count}")
            count += 1

level_sub.save_current_level()
unreal.log(f"=== Spawned {count} extras (snipers, crates, barrels, fences) ===")
