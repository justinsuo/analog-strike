"""
ANALOG STRIKE — PROCEDURAL MAP BUILDER
Builds a cohesive military facility using Kenney modular building pieces.
Imports the prototype kit (walls/floors/doors) + tower defense details (trees/rocks),
then constructs real buildings, a perimeter wall, watchtowers, roads, and scatters detail.

Run inside the UE5 editor:  py /tmp/ue_build_map.py
"""
import unreal
import os
import math
import random

random.seed(7)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

PROTO = os.path.expanduser("~/Downloads/kenney_models/prototype-kit/extracted")
TD = os.path.expanduser("~/Downloads/kenney_models/tower-defense-kit/extracted")
DEST = "/Game/AnalogStrike/MapKit"

# Find the actual FBX directories (they're nested under a "Models/FBX format" path)
def find_fbx_dir(base):
    for root, dirs, files in os.walk(base):
        if any(f.endswith(".fbx") for f in files):
            return root
    return None

proto_dir = find_fbx_dir(PROTO)
td_dir = find_fbx_dir(TD)

# ──────────────────────────────────────────────────────────────────────
# STEP 1: Import only the pieces we need
# ──────────────────────────────────────────────────────────────────────
needed = {
    "wall": proto_dir, "wall-corner": proto_dir, "wall-doorway-wide": proto_dir,
    "wall-window-large": proto_dir, "floor-square": proto_dir, "floor-thick": proto_dir,
    "column": proto_dir, "stairs": proto_dir, "crate": proto_dir, "crate-color": proto_dir,
    "door-sliding": proto_dir, "wall-low": proto_dir,
    "detail-tree-large": td_dir, "detail-tree": td_dir,
    "detail-rocks-large": td_dir, "detail-rocks": td_dir, "detail-crystal": td_dir,
}

mesh_cache = {}

def import_piece(name, src_dir):
    if name in mesh_cache:
        return mesh_cache[name]
    if not src_dir:
        return None
    fp = os.path.join(src_dir, name + ".fbx")
    if not os.path.exists(fp):
        unreal.log_warning(f"Missing: {fp}")
        mesh_cache[name] = None
        return None
    # Already imported?
    asset_path = f"{DEST}/{name}"
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing and isinstance(existing, unreal.StaticMesh):
        mesh_cache[name] = existing
        return existing
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fp)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    opt = unreal.FbxImportUI()
    opt.set_editor_property("import_mesh", True)
    opt.set_editor_property("import_materials", True)
    opt.set_editor_property("import_as_skeletal", False)
    opt.static_mesh_import_data.set_editor_property("combine_meshes", True)
    task.set_editor_property("options", opt)
    asset_tools.import_asset_tasks([task])
    # Locate result
    for a in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
        if a.split("/")[-1].split(".")[0] == name:
            m = unreal.EditorAssetLibrary.load_asset(a)
            if isinstance(m, unreal.StaticMesh):
                mesh_cache[name] = m
                return m
    mesh_cache[name] = None
    return None

unreal.log("Importing map kit pieces...")
for name, d in needed.items():
    import_piece(name, d)

# Kenney prototype pieces are ~1m (100uu) footprint when imported at scale 1.
# We scale them up to make a chunky, readable facility.
SCALE = 200.0          # visual scale multiplier
TILE = 200.0           # grid spacing in world units (matches scaled footprint)
placed = 0

def place(mesh_name, x, y, z, yaw=0.0, scale=SCALE, label=None):
    global placed
    m = mesh_cache.get(mesh_name)
    if not m:
        return None
    loc = unreal.Vector(x, y, z)
    rot = unreal.Rotator(0, yaw, 0)
    a = editor.spawn_actor_from_object(m, loc, rot)
    if a:
        a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        a.set_actor_label(label or f"{mesh_name}_{placed}")
        placed += 1
    return a

# ──────────────────────────────────────────────────────────────────────
# STEP 2: Building generator — builds a walled rectangular structure
# ──────────────────────────────────────────────────────────────────────
def build_building(cx, cy, cols, rows, floor_z=0, with_floor=True, door_side=0):
    """Build a rectangular building footprint cols x rows tiles centered at (cx,cy)."""
    half_w = (cols - 1) * TILE / 2
    half_d = (rows - 1) * TILE / 2

    # Floor
    if with_floor:
        for i in range(cols):
            for j in range(rows):
                fx = cx - half_w + i * TILE
                fy = cy - half_d + j * TILE
                place("floor-square", fx, fy, floor_z, 0)

    # Walls around perimeter
    for i in range(cols):
        fx = cx - half_w + i * TILE
        # Front wall (door_side 0 gets a doorway in the middle)
        front_piece = "wall-doorway-wide" if (door_side == 0 and i == cols // 2) else \
                      ("wall-window-large" if i % 2 == 0 else "wall")
        place(front_piece, fx, cy - half_d - TILE/2, floor_z, 0)
        # Back wall
        back_piece = "wall-doorway-wide" if (door_side == 2 and i == cols // 2) else \
                     ("wall-window-large" if i % 2 == 1 else "wall")
        place(back_piece, fx, cy + half_d + TILE/2, floor_z, 180)

    for j in range(rows):
        fy = cy - half_d + j * TILE
        left_piece = "wall-doorway-wide" if (door_side == 1 and j == rows // 2) else \
                     ("wall-window-large" if j % 2 == 0 else "wall")
        place(left_piece, cx - half_w - TILE/2, fy, floor_z, 90)
        right_piece = "wall-doorway-wide" if (door_side == 3 and j == rows // 2) else \
                      ("wall-window-large" if j % 2 == 1 else "wall")
        place(right_piece, cx + half_w + TILE/2, fy, floor_z, 270)

    # Corner columns
    for sx in (-1, 1):
        for sy in (-1, 1):
            place("column", cx + sx*(half_w+TILE/2), cy + sy*(half_d+TILE/2), floor_z, 0)

# ──────────────────────────────────────────────────────────────────────
# STEP 3: Lay out the facility — distinct zones
# ──────────────────────────────────────────────────────────────────────
unreal.log("Building facility structures...")

# Command Center (large, central) — at relay node area
build_building(5500, 3200, 6, 5, floor_z=0, door_side=0)

# Barracks cluster (near spawn / west)
build_building(-2500, 0, 4, 3, door_side=0)
build_building(-2500, 900, 4, 3, door_side=0)
build_building(-1600, 0, 3, 3, door_side=1)

# Warehouses (research station / east)
build_building(4200, 0, 5, 4, door_side=2)
build_building(4200, 1200, 4, 4, door_side=0)

# Checkpoint guard posts (south)
build_building(3000, -3200, 3, 3, door_side=0)
build_building(2200, -3200, 2, 2, door_side=0)

# Bunker (small, fortified, south-west)
build_building(0, -5200, 3, 3, door_side=0)

# ──────────────────────────────────────────────────────────────────────
# STEP 4: Perimeter low wall around the central compound + watchtowers
# ──────────────────────────────────────────────────────────────────────
unreal.log("Building perimeter...")
perim_cx, perim_cy, perim_r = 2000, 0, 7000
seg = 0
steps = 48
for s in range(steps):
    ang = (s / steps) * math.tau
    px = perim_cx + math.cos(ang) * perim_r
    py = perim_cy + math.sin(ang) * perim_r
    yaw = math.degrees(ang) + 90
    place("wall-low", px, py, 0, yaw, scale=SCALE*1.2)
    seg += 1

# Watchtowers at cardinal points (stacked crates + column)
for ang_deg in (0, 90, 180, 270):
    ang = math.radians(ang_deg)
    tx = perim_cx + math.cos(ang) * perim_r
    ty = perim_cy + math.sin(ang) * perim_r
    for h in range(3):
        place("column", tx, ty, h * TILE, 0, scale=SCALE*1.3)
    place("floor-thick", tx, ty, 3*TILE, 0, scale=SCALE*1.5)

# ──────────────────────────────────────────────────────────────────────
# STEP 5: Tree & rock scatter for natural terrain
# ──────────────────────────────────────────────────────────────────────
unreal.log("Scattering vegetation and rocks...")
# Forest zone (west)
for _ in range(60):
    x = random.uniform(-6000, -3500)
    y = random.uniform(-3000, 3000)
    t = "detail-tree-large" if random.random() < 0.5 else "detail-tree"
    place(t, x, y, 0, random.uniform(0, 360), scale=SCALE*random.uniform(1.5, 2.5))

# Scattered trees across the map
for _ in range(50):
    x = random.uniform(-7000, 8000)
    y = random.uniform(-6000, 7000)
    # Avoid the central compound
    if abs(x - 2000) < 1500 and abs(y) < 1500:
        continue
    place("detail-tree", x, y, 0, random.uniform(0, 360), scale=SCALE*random.uniform(1.2, 2.0))

# Rock formations
for _ in range(40):
    x = random.uniform(-7000, 8000)
    y = random.uniform(-6000, 7000)
    r = "detail-rocks-large" if random.random() < 0.4 else "detail-rocks"
    place(r, x, y, 0, random.uniform(0, 360), scale=SCALE*random.uniform(1.5, 3.0))

# Crystal accents near the relay/objective for a sci-fi look
for _ in range(12):
    ang = random.uniform(0, math.tau)
    d = random.uniform(400, 1200)
    place("detail-crystal", 5500 + math.cos(ang)*d, 3200 + math.sin(ang)*d, 0,
          random.uniform(0, 360), scale=SCALE*random.uniform(1.0, 2.0))

# ──────────────────────────────────────────────────────────────────────
# STEP 6: Crate cover scattered around combat zones
# ──────────────────────────────────────────────────────────────────────
unreal.log("Placing cover crates...")
combat_zones = [(5500, 3200), (4200, 0), (3000, -3200), (-2500, 0), (0, -5200), (1500, 1500)]
for (zx, zy) in combat_zones:
    for _ in range(8):
        cx = zx + random.uniform(-600, 600)
        cy = zy + random.uniform(-600, 600)
        c = "crate-color" if random.random() < 0.5 else "crate"
        # Stack some crates
        stack = random.randint(1, 2)
        for h in range(stack):
            place(c, cx, cy, h * TILE * 0.5, random.uniform(0, 360), scale=SCALE*0.5)

level_sub.save_current_level()
unreal.log("══════════════════════════════════════════")
unreal.log(f"MAP BUILD COMPLETE — {placed} objects placed")
unreal.log("Zones: Command Center, Barracks, Warehouses,")
unreal.log("Checkpoint, Bunker, perimeter wall, watchtowers,")
unreal.log("forest, rock formations, crate cover.")
unreal.log("══════════════════════════════════════════")
