"""Populate the OpenWorld with physics props, ammo crates, enemies, hazards. Run in UE5 editor."""
import unreal
import random

level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")
editor_lib = unreal.EditorLevelLibrary
count = 0

# ── PHYSICS PROPS (rigid bodies) ──────────────────────────────────────
prop_class = unreal.load_class(None, "/Script/AnalogStrike.ASPhysicsProp")
if prop_class:
    # Crates scattered around buildings and checkpoints
    prop_positions = []
    # Near FOB/spawn
    for i in range(8):
        prop_positions.append((random.uniform(-500, 200), random.uniform(-300, 300), 30, 0))
    # Checkpoint area
    for i in range(10):
        prop_positions.append((random.uniform(2800, 3500), random.uniform(-3500, -2800), 30, random.randint(0, 4)))
    # Research station
    for i in range(12):
        prop_positions.append((random.uniform(3800, 5000), random.uniform(-500, 500), 30, random.randint(0, 4)))
    # Pine forest
    for i in range(6):
        prop_positions.append((random.uniform(-5000, -3800), random.uniform(-500, 1000), 30, random.randint(2, 3)))
    # Cave system
    for i in range(8):
        prop_positions.append((random.uniform(4800, 5800), random.uniform(2800, 3800), 30, random.randint(0, 2)))
    # Central valley
    for i in range(10):
        prop_positions.append((random.uniform(-1000, 2000), random.uniform(-1000, 2000), 30, random.randint(0, 4)))
    # Bunker entrance
    for i in range(8):
        prop_positions.append((random.uniform(-500, 500), random.uniform(-5500, -4800), 30, random.randint(0, 2)))

    for pos in prop_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        rot = unreal.Rotator(0, random.uniform(0, 360), 0)
        actor = editor_lib.spawn_actor_from_class(prop_class, loc, rot)
        if actor:
            # Set prop type via property
            actor.set_editor_property("prop_type", pos[3])
            actor.set_actor_label(f"PhysicsProp_{count}")
            count += 1

# ── AMMO CRATES ──────────────────────────────────────────────────────
crate_class = unreal.load_class(None, "/Script/AnalogStrike.ASAmmoCrate")
if crate_class:
    crate_positions = [
        (-200, -100, 20), (1000, 0, 20), (2500, -2500, 20),
        (4500, 200, 20), (5000, 3500, 20), (-4000, 0, 20),
        (0, 6000, 20), (6500, 0, 20),
    ]
    for pos in crate_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(crate_class, loc, unreal.Rotator(0, 0, 0))
        if actor:
            actor.set_actor_label(f"AmmoCrate_{count}")
            count += 1

# ── EXPLOSIVE BARRELS ────────────────────────────────────────────────
barrel_class = unreal.load_class(None, "/Script/AnalogStrike.ASExplosiveBarrel")
if barrel_class:
    barrel_positions = [
        (3200, -3200, 10), (3100, -3100, 10), (4500, 500, 10),
        (5200, 3200, 10), (1500, 1500, 10), (-500, 2000, 10),
        (-4500, 500, 10), (6500, 100, 10), (500, -5200, 10),
        (2000, -1000, 10), (4200, -200, 10), (-4800, 800, 10),
    ]
    for pos in barrel_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(barrel_class, loc, unreal.Rotator(0, random.uniform(0, 360), 0))
        if actor:
            actor.set_actor_label(f"Barrel_{count}")
            count += 1

# ── SNIPERS ──────────────────────────────────────────────────────────
sniper_class = unreal.load_class(None, "/Script/AnalogStrike.ASSniper")
if sniper_class:
    sniper_positions = [
        (5000, 2000, 200), (-3000, 4000, 150), (7000, -1000, 200),
        (2000, 6000, 180), (-5000, -2000, 150), (4000, -4000, 200),
    ]
    for pos in sniper_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(sniper_class, loc, unreal.Rotator(0, 0, 0))
        if actor:
            actor.set_actor_label(f"Sniper_{count}")
            count += 1

# ── ELECTRIC FENCES ──────────────────────────────────────────────────
fence_class = unreal.load_class(None, "/Script/AnalogStrike.ASElectricFence")
if fence_class:
    fence_data = [
        (4000, 1000, 10, 0), (4000, -1000, 10, 0), (4800, 0, 10, 90),
        (5000, 3000, 10, 45), (3000, -3500, 10, 30), (3500, -3500, 10, -30),
    ]
    for pos in fence_data:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        rot = unreal.Rotator(0, pos[3], 0)
        actor = editor_lib.spawn_actor_from_class(fence_class, loc, rot)
        if actor:
            actor.set_actor_label(f"Fence_{count}")
            count += 1

# ── SECURITY FRAMES (ranged enemies) ─────────────────────────────────
sec_class = unreal.load_class(None, "/Script/AnalogStrike.ASSecurityFrame")
if sec_class:
    sec_positions = [
        (1500, 500, 10), (2000, -1500, 10), (3500, -2500, 10),
        (4000, 0, 10), (4500, 3000, 10), (-2000, 1000, 10),
        (-3500, -1000, 10), (500, -4000, 10), (5500, 500, 10),
        (6000, -500, 10), (1000, 3000, 10), (-1500, -2500, 10),
    ]
    for pos in sec_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(sec_class, loc, unreal.Rotator(0, random.uniform(0, 360), 0))
        if actor:
            actor.set_actor_label(f"SecurityFrame_{count}")
            count += 1

# ── MELEE ENEMIES ────────────────────────────────────────────────────
enemy_class = unreal.load_class(None, "/Script/AnalogStrike.ASEnemyBase")
if enemy_class:
    enemy_positions = [
        (800, 200, 10), (1200, -800, 10), (2500, -2000, 10),
        (3000, 500, 10), (-1500, 500, 10), (-2500, -500, 10),
        (1000, 2000, 10), (3500, 3000, 10), (-500, -3000, 10),
        (4500, -1000, 10),
    ]
    for pos in enemy_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(enemy_class, loc, unreal.Rotator(0, random.uniform(0, 360), 0))
        if actor:
            actor.set_actor_label(f"Enemy_{count}")
            count += 1

# ── SCOUT DRONES ─────────────────────────────────────────────────────
drone_class = unreal.load_class(None, "/Script/AnalogStrike.ASScoutDrone")
if drone_class:
    drone_positions = [
        (2000, 1000, 300), (4000, 2000, 350), (-1000, 3000, 280),
        (5500, 3500, 320), (0, -3000, 300),
    ]
    for pos in drone_positions:
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(drone_class, loc, unreal.Rotator(0, 0, 0))
        if actor:
            actor.set_actor_label(f"Drone_{count}")
            count += 1

# ── NPCS ─────────────────────────────────────────────────────────────
npc_class = unreal.load_class(None, "/Script/AnalogStrike.ASNPC")
if npc_class:
    npc_positions = [
        (-400, -200, 10),  # Commander near spawn
        (-350, -100, 10),  # Tech near spawn
        (-300, 0, 10),     # Medic near spawn
        (6800, 200, 10),   # Scout near extraction
    ]
    for idx, pos in enumerate(npc_positions):
        loc = unreal.Vector(pos[0], pos[1], pos[2])
        actor = editor_lib.spawn_actor_from_class(npc_class, loc, unreal.Rotator(0, 90, 0))
        if actor:
            roles = ["Commander", "Technician", "Medic", "Scout"]
            actor.set_actor_label(f"NPC_{roles[idx]}_{count}")
            # Set role via property
            actor.set_editor_property("npc_role", idx)
            count += 1

# ── RELAY NODE ───────────────────────────────────────────────────────
relay_class = unreal.load_class(None, "/Script/AnalogStrike.ASRelayNode")
if relay_class:
    actor = editor_lib.spawn_actor_from_class(relay_class, unreal.Vector(5500, 3500, 50), unreal.Rotator(0, 0, 0))
    if actor:
        actor.set_actor_label("RelayNode_OBJECTIVE")
        count += 1

# ── EXTRACTION ZONE ──────────────────────────────────────────────────
extract_class = unreal.load_class(None, "/Script/AnalogStrike.ASExtractionZone")
if extract_class:
    actor = editor_lib.spawn_actor_from_class(extract_class, unreal.Vector(7000, 0, 10), unreal.Rotator(0, 0, 0))
    if actor:
        actor.set_actor_label("ExtractionZone_HELIPAD")
        count += 1

level_sub.save_current_level()
unreal.log(f"=== POPULATED WORLD: {count} actors placed ===")

# ── WEATHER SYSTEM ───────────────────────────────────────────────────
weather_class = unreal.load_class(None, "/Script/AnalogStrike.ASWeatherSystem")
if weather_class:
    actor = editor_lib.spawn_actor_from_class(weather_class, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if actor:
        actor.set_actor_label("WeatherSystem")
        count += 1
        unreal.log("Placed weather system")

# ── WARDEN BOSS ──────────────────────────────────────────────────────
warden_class = unreal.load_class(None, "/Script/AnalogStrike.ASWarden")
if warden_class:
    actor = editor_lib.spawn_actor_from_class(warden_class, unreal.Vector(5500, 3200, 50), unreal.Rotator(0, 0, 0))
    if actor:
        actor.set_actor_label("BOSS_Warden")
        count += 1
        unreal.log("Placed Warden boss near relay node")

level_sub.save_current_level()
unreal.log(f"=== FINAL COUNT: {count} actors ===")
