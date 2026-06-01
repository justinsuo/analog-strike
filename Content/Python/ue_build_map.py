"""
ANALOG STRIKE — OPEN-WORLD MAP BUILDER
Reads the POI list + terrain heights from as_terrain_heights.json and builds a
distinct settlement at each point of interest using the Kenney modular kit, with
a road network, perimeter defenses, and vegetation across the landscape.

Run inside the UE5 editor:  py /tmp/ue_build_map.py
(Run ue_import_terrain.py first so buildings sit on the terrain surface.)
"""
import unreal
import os, math, random, json

random.seed(7)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor = unreal.EditorLevelLibrary
level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_sub.load_level("/Game/AnalogStrike/Maps/OpenWorld")

PROTO = os.path.expanduser("~/Downloads/kenney_models/prototype-kit/extracted")
TD = os.path.expanduser("~/Downloads/kenney_models/tower-defense-kit/extracted")
DEST = "/Game/AnalogStrike/MapKit"

def find_fbx_dir(base):
    for root, dirs, files in os.walk(base):
        if any(f.endswith(".fbx") for f in files):
            return root
    return None
proto_dir = find_fbx_dir(PROTO)
td_dir = find_fbx_dir(TD)

# ── Import the kit pieces ──
needed = {
    "wall": proto_dir, "wall-corner": proto_dir, "wall-doorway-wide": proto_dir,
    "wall-window-large": proto_dir, "wall-low": proto_dir, "floor-square": proto_dir,
    "floor-thick": proto_dir, "column": proto_dir, "stairs": proto_dir,
    "crate": proto_dir, "crate-color": proto_dir, "door-sliding": proto_dir,
    "detail-tree-large": td_dir, "detail-tree": td_dir,
    "detail-rocks-large": td_dir, "detail-rocks": td_dir, "detail-crystal": td_dir,
}
mesh_cache = {}
def import_piece(name, src_dir):
    if name in mesh_cache: return mesh_cache[name]
    if not src_dir: return None
    fp = os.path.join(src_dir, name + ".fbx")
    if not os.path.exists(fp):
        unreal.log_warning(f"Missing: {fp}"); mesh_cache[name] = None; return None
    existing = unreal.EditorAssetLibrary.load_asset(f"{DEST}/{name}")
    if existing and isinstance(existing, unreal.StaticMesh):
        mesh_cache[name] = existing; return existing
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
    for a in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
        if a.split("/")[-1].split(".")[0] == name:
            m = unreal.EditorAssetLibrary.load_asset(a)
            if isinstance(m, unreal.StaticMesh):
                mesh_cache[name] = m; return m
    mesh_cache[name] = None; return None

unreal.log("Importing map kit pieces...")
for nm, d in needed.items():
    import_piece(nm, d)

SCALE = 380.0
TILE = 380.0
placed = 0

# ── Terrain height + POI data ──
_TH = None
try:
    with open(os.path.expanduser("~/Downloads/as_terrain_heights.json")) as _f:
        _TH = json.load(_f)
except Exception as e:
    unreal.log_warning(f"No terrain data ({e}); flat z=0.")
POIS = _TH.get("pois", []) if _TH else []

def ground_z(x, y):
    if not _TH: return 0.0
    n, world, grid = _TH["n"], _TH["world"], _TH["grid"]
    half = world / 2.0
    fx = (x + half) / world * (n - 1); fy = (y + half) / world * (n - 1)
    if fx < 0 or fx > n-1 or fy < 0 or fy > n-1: return 0.0
    i0, j0 = int(fx), int(fy); i1, j1 = min(i0+1, n-1), min(j0+1, n-1)
    tx, ty = fx-i0, fy-j0
    h00, h10 = grid[j0][i0], grid[j0][i1]; h01, h11 = grid[j1][i0], grid[j1][i1]
    return (h00*(1-tx)+h10*tx)*(1-ty) + (h01*(1-tx)+h11*tx)*ty

def place_raw(name, x, y, z_abs, yaw=0.0, scale=SCALE, label=None):
    global placed
    m = mesh_cache.get(name)
    if not m: return None
    a = editor.spawn_actor_from_object(m, unreal.Vector(x, y, z_abs), unreal.Rotator(0, yaw, 0))
    if a:
        a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        a.set_actor_label(label or f"{name}_{placed}"); placed += 1
    return a

def place(name, x, y, z, yaw=0.0, scale=SCALE, label=None):
    return place_raw(name, x, y, ground_z(x, y) + z, yaw, scale, label)

# ── Building generator (flat at the terrain height of its center) ──
def build_building(cx, cy, cols, rows, door_side=0, ruined=False, floor_z=None):
    if floor_z is None: floor_z = ground_z(cx, cy)
    half_w = (cols-1)*TILE/2; half_d = (rows-1)*TILE/2
    # Floor
    for i in range(cols):
        for j in range(rows):
            place_raw("floor-square", cx-half_w+i*TILE, cy-half_d+j*TILE, floor_z, 0)
    # Walls — for ruined buildings, randomly skip/lower pieces
    def wpiece(default):
        if ruined:
            r = random.random()
            if r < 0.3: return None          # gap
            if r < 0.6: return "wall-low"     # broken low wall
        return default
    for i in range(cols):
        fx = cx-half_w+i*TILE
        fp = "wall-doorway-wide" if (door_side==0 and i==cols//2) else ("wall-window-large" if i%2==0 else "wall")
        p = wpiece(fp);  place_raw(p, fx, cy-half_d-TILE/2, floor_z, 0) if p else None
        bp = "wall-doorway-wide" if (door_side==2 and i==cols//2) else ("wall-window-large" if i%2==1 else "wall")
        p = wpiece(bp);  place_raw(p, fx, cy+half_d+TILE/2, floor_z, 180) if p else None
    for j in range(rows):
        fy = cy-half_d+j*TILE
        lp = "wall-doorway-wide" if (door_side==1 and j==rows//2) else ("wall-window-large" if j%2==0 else "wall")
        p = wpiece(lp);  place_raw(p, cx-half_w-TILE/2, fy, floor_z, 90) if p else None
        rp = "wall-doorway-wide" if (door_side==3 and j==rows//2) else ("wall-window-large" if j%2==1 else "wall")
        p = wpiece(rp);  place_raw(p, cx+half_w+TILE/2, fy, floor_z, 270) if p else None
    # Corner columns
    for sx in (-1, 1):
        for sy in (-1, 1):
            place_raw("column", cx+sx*(half_w+TILE/2), cy+sy*(half_d+TILE/2), floor_z, 0)

# ── Roads ──
def road(x1, y1, x2, y2, width_tiles=2):
    dist = math.hypot(x2-x1, y2-y1)
    steps = max(1, int(dist/(TILE*0.9)))
    dx, dy = (x2-x1)/steps, (y2-y1)/steps
    plen = math.hypot(dx, dy) or 1
    perp = (-dy/plen, dx/plen)
    for s in range(steps+1):
        rx, ry = x1+dx*s, y1+dy*s
        for w in range(width_tiles):
            off = (w-(width_tiles-1)/2)*TILE
            place("floor-square", rx+perp[0]*off, ry+perp[1]*off, 4, 0, scale=SCALE)

def crates(cx, cy, rad, count=6):
    for _ in range(count):
        x = cx+random.uniform(-rad, rad); y = cy+random.uniform(-rad, rad)
        c = "crate-color" if random.random()<0.5 else "crate"
        for h in range(random.randint(1, 2)):
            place_raw(c, x, y, ground_z(x,y)+h*TILE*0.5, random.uniform(0,360), scale=SCALE*0.55)

def perimeter(cx, cy, rad, towers=True):
    steps = max(16, int(2*math.pi*rad/(TILE*1.2)))
    for s in range(steps):
        a = s/steps*math.tau
        place("wall-low", cx+math.cos(a)*rad, cy+math.sin(a)*rad, 0, math.degrees(a)+90, scale=SCALE*1.2)
    if towers:
        for ad in range(0, 360, 90):
            a = math.radians(ad); tx, ty = cx+math.cos(a)*rad, cy+math.sin(a)*rad
            bz = ground_z(tx, ty)
            for h in range(3): place_raw("column", tx, ty, bz+h*TILE, 0, scale=SCALE*1.3)
            place_raw("floor-thick", tx, ty, bz+3*TILE, 0, scale=SCALE*1.5)

# ──────────────────────────────────────────────────────────────────────
# SETTLEMENT TEMPLATES — one per POI type
# ──────────────────────────────────────────────────────────────────────
def settle_main_base(cx, cy):
    build_building(cx, cy+1900, 8, 6, door_side=2)        # command center
    build_building(cx-2400, cy+200, 5, 4, door_side=3)    # barracks
    build_building(cx-2400, cy+1400, 5, 4, door_side=3)
    build_building(cx+2400, cy+200, 6, 4, door_side=1)    # warehouses
    build_building(cx+2400, cy+1400, 5, 4, door_side=1)
    build_building(cx, cy-2000, 5, 3, door_side=0)        # motor pool
    # central plaza
    for i in range(-2, 3):
        for j in range(-2, 3):
            place_raw("floor-thick", cx+i*TILE, cy+j*TILE, ground_z(cx, cy), 0)
    perimeter(cx, cy, 4200, towers=True)
    for z in [(cx,cy+1900),(cx-2400,cy+800),(cx+2400,cy+800),(cx,cy-2000)]:
        crates(z[0], z[1], 700, 7)
    for _ in range(10):
        a=random.uniform(0,math.tau); d=random.uniform(300,900)
        place("detail-crystal", cx+math.cos(a)*d, cy+1900+math.sin(a)*d, 0, random.uniform(0,360), scale=SCALE*1.4)

def settle_outpost(cx, cy):
    build_building(cx, cy, 4, 3, door_side=0)
    build_building(cx+1500, cy-600, 3, 3, door_side=1)
    bz = ground_z(cx-1400, cy+400)
    for h in range(3): place_raw("column", cx-1400, cy+400, bz+h*TILE, 0, scale=SCALE*1.3)  # watchtower
    place_raw("floor-thick", cx-1400, cy+400, bz+3*TILE, 0, scale=SCALE*1.5)
    perimeter(cx, cy, 2200, towers=False)
    crates(cx, cy, 600, 5)

def settle_ruined_town(cx, cy):
    spots = [(-2200,-1500),(-700,-1800),(900,-1200),(2200,-200),(1200,1400),
             (-400,1700),(-1900,900),(300,200)]
    for (dx, dy) in spots:
        cols, rows = random.choice([(3,3),(4,3),(3,4),(2,3)])
        build_building(cx+dx, cy+dy, cols, rows, door_side=random.randint(0,3), ruined=True)
    # rubble
    for _ in range(25):
        x=cx+random.uniform(-2800,2800); y=cy+random.uniform(-2800,2800)
        place("detail-rocks", x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(0.6,1.4))
    crates(cx, cy, 2200, 10)

def settle_research(cx, cy):
    build_building(cx, cy, 5, 4, door_side=0)            # main lab
    build_building(cx-1700, cy+200, 3, 3, door_side=1)
    build_building(cx+1700, cy-200, 4, 3, door_side=3)
    # antenna columns
    for (dx,dy) in [(-1000,1300),(1000,1300)]:
        bz=ground_z(cx+dx,cy+dy)
        for h in range(4): place_raw("column", cx+dx, cy+dy, bz+h*TILE, 0, scale=SCALE*0.8)
    perimeter(cx, cy, 2300, towers=True)
    crates(cx, cy, 900, 6)

def settle_checkpoint(cx, cy):
    build_building(cx, cy, 5, 3, door_side=0)            # gatehouse
    # gate walls flanking a road gap (north-south road passes through)
    for off in range(1, 5):
        place("wall", cx-700-off*0, cy+off*TILE, 0, 90, scale=SCALE)
        place("wall", cx+700, cy+off*TILE, 0, 90, scale=SCALE)
    for h in range(3):
        place_raw("column", cx-900, cy, ground_z(cx-900,cy)+h*TILE, 0, scale=SCALE*1.2)
        place_raw("column", cx+900, cy, ground_z(cx+900,cy)+h*TILE, 0, scale=SCALE*1.2)
    crates(cx, cy, 800, 5)

def settle_bunker(cx, cy):
    build_building(cx, cy, 4, 4, door_side=0)            # fortified
    perimeter(cx, cy, 1500, towers=True)
    # sandbag-like crate walls
    for ad in range(0, 360, 30):
        a=math.radians(ad); x=cx+math.cos(a)*1100; y=cy+math.sin(a)*1100
        place_raw("crate", x, y, ground_z(x,y), math.degrees(a), scale=SCALE*0.6)
    crates(cx, cy, 700, 5)

BUILDERS = {
    "MainBase": settle_main_base, "ForwardOutpost": settle_outpost,
    "RuinedTown": settle_ruined_town, "ResearchStation": settle_research,
    "Checkpoint": settle_checkpoint, "MountainBunker": settle_bunker,
}

# ──────────────────────────────────────────────────────────────────────
# BUILD: settlements, roads, vegetation
# ──────────────────────────────────────────────────────────────────────
unreal.log(f"Building {len(POIS)} settlements...")
main = next((p for p in POIS if p["name"] == "MainBase"), None)
for p in POIS:
    fn = BUILDERS.get(p["name"])
    if fn:
        unreal.log(f"  {p['name']} at ({p['x']},{p['y']})")
        fn(p["x"], p["y"])

# Roads: hub-and-spoke from Main Base to each settlement
if main:
    for p in POIS:
        if p["name"] == "MainBase": continue
        road(main["x"], main["y"], p["x"], p["y"], width_tiles=2)

# Vegetation across the landscape (avoid settlement pads)
unreal.log("Scattering vegetation...")
def near_poi(x, y):
    for p in POIS:
        if math.hypot(x-p["x"], y-p["y"]) < p["r"]*0.8:
            return True
    return False
HALF = (_TH["world"]/2) if _TH else 25000
for _ in range(400):
    x = random.uniform(-HALF*0.92, HALF*0.92); y = random.uniform(-HALF*0.92, HALF*0.92)
    if near_poi(x, y): continue
    roll = random.random()
    if roll < 0.55:
        t = "detail-tree-large" if random.random()<0.5 else "detail-tree"
        place(t, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.5,3.0))
    elif roll < 0.85:
        r = "detail-rocks-large" if random.random()<0.4 else "detail-rocks"
        place(r, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.8,3.5))

level_sub.save_current_level()
unreal.log("══════════════════════════════════════════")
unreal.log(f"OPEN-WORLD BUILD COMPLETE — {placed} objects, {len(POIS)} settlements")
unreal.log("══════════════════════════════════════════")
