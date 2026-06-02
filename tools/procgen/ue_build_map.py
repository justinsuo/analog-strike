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

KIT = lambda name: os.path.expanduser(f"~/Downloads/kenney_models/{name}")
PROTO   = KIT("prototype-kit/extracted")
TD      = KIT("tower-defense-kit/extracted")
NATURE  = KIT("nature-kit")
ROADS   = KIT("city-kit-roads")
DEST    = "/Game/AnalogStrike/MapKit"

def find_fbx_dir(base):
    for root, dirs, files in os.walk(base):
        if any(f.endswith(".fbx") for f in files):
            return root
    return None
proto_dir  = find_fbx_dir(PROTO)
td_dir     = find_fbx_dir(TD)
nature_dir = find_fbx_dir(NATURE)
road_dir   = find_fbx_dir(ROADS)

# ── Import the kit pieces ──
needed = {
    # Prototype kit — buildings
    "wall": proto_dir, "wall-corner": proto_dir, "wall-doorway-wide": proto_dir,
    "wall-window-large": proto_dir, "wall-low": proto_dir, "floor-square": proto_dir,
    "floor-thick": proto_dir, "column": proto_dir, "stairs": proto_dir,
    "crate": proto_dir, "crate-color": proto_dir, "door-sliding": proto_dir,
    # Tower defense — base trees/rocks (kept as fallback)
    "detail-tree-large": td_dir, "detail-tree": td_dir,
    "detail-rocks-large": td_dir, "detail-rocks": td_dir, "detail-crystal": td_dir,
    # Nature kit — 8 tree variants, multiple rocks (much more variety)
    "tree_default": nature_dir, "tree_oak": nature_dir, "tree_fat": nature_dir,
    "tree_cone": nature_dir, "tree_detailed": nature_dir, "tree_palmTall": nature_dir,
    "tree_default_dark": nature_dir, "tree_oak_dark": nature_dir, "tree_fat_dark": nature_dir,
    "rock_largeA": nature_dir, "rock_largeB": nature_dir, "rock_largeC": nature_dir,
    "rock_smallA": nature_dir, "rock_smallB": nature_dir, "rock_smallC": nature_dir,
    # City roads kit — real road segments + streetlamps + bridges
    "road-straight": road_dir, "road-bend": road_dir, "road-crossroad": road_dir,
    "road-intersection": road_dir, "road-bridge": road_dir,
    "light-square": road_dir, "light-curved": road_dir, "construction-barrier": road_dir,
}
NATURE_TREES = ["tree_default", "tree_oak", "tree_fat", "tree_cone", "tree_detailed",
                "tree_default_dark", "tree_oak_dark", "tree_fat_dark", "tree_palmTall"]
NATURE_ROCKS_LARGE = ["rock_largeA", "rock_largeB", "rock_largeC"]
NATURE_ROCKS_SMALL = ["rock_smallA", "rock_smallB", "rock_smallC"]
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

# ── Roads ── use real road-straight segments from city-kit-roads + streetlamps
# Falls back to floor-square tiles if the road kit didn't import.
def road(x1, y1, x2, y2, width_tiles=2):
    dist = math.hypot(x2-x1, y2-y1)
    steps = max(1, int(dist/(TILE*0.9)))
    dx, dy = (x2-x1)/steps, (y2-y1)/steps
    angle = math.degrees(math.atan2(dy, dx))
    plen = math.hypot(dx, dy) or 1
    perp = (-dy/plen, dx/plen)
    has_road_kit = mesh_cache.get("road-straight") is not None
    has_lamp_kit = mesh_cache.get("light-square") is not None
    for s in range(steps+1):
        rx, ry = x1+dx*s, y1+dy*s
        if has_road_kit:
            # One real road-straight oriented along the road, tile-spaced
            place("road-straight", rx, ry, 6, angle, scale=SCALE)
        else:
            # Floor-tile fallback (old behaviour)
            for w in range(width_tiles):
                off = (w-(width_tiles-1)/2)*TILE
                place("floor-square", rx+perp[0]*off, ry+perp[1]*off, 4, 0, scale=SCALE)
        # Streetlamp every 6 tiles along the side of the road
        if has_lamp_kit and s > 0 and s % 6 == 0:
            lx = rx + perp[0]*TILE*1.2; ly = ry + perp[1]*TILE*1.2
            place("light-square", lx, ly, 0, angle, scale=SCALE)

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
    for ad in range(0, 360, 30):
        a=math.radians(ad); x=cx+math.cos(a)*1100; y=cy+math.sin(a)*1100
        place_raw("crate", x, y, ground_z(x,y), math.degrees(a), scale=SCALE*0.6)
    crates(cx, cy, 700, 5)

def settle_quarry(cx, cy):
    # Open pit with industrial buildings
    build_building(cx-1200, cy-1000, 5, 4, door_side=2)
    build_building(cx+1400, cy+800, 4, 3, door_side=1)
    # Lots of rocks scattered (the quarry pile)
    for _ in range(28):
        x = cx + random.uniform(-1800, 1800); y = cy + random.uniform(-1800, 1800)
        if math.hypot(x-cx, y-cy) < 700: continue  # leave pit area clear
        r = "detail-rocks-large" if random.random()<0.5 else "detail-rocks"
        place(r, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.5, 3.2))
    crates(cx, cy, 1200, 8)

def settle_comm_array(cx, cy):
    # Small command + tall antenna stack
    build_building(cx, cy, 3, 3, door_side=0)
    build_building(cx-1100, cy+800, 3, 3, door_side=3)
    # Antenna: 6 stacked columns (tall)
    for (ax, ay) in [(cx+1300, cy), (cx+1100, cy-1200), (cx-1300, cy-800)]:
        bz = ground_z(ax, ay)
        for hi in range(6):
            place_raw("column", ax, ay, bz + hi*TILE, 0, scale=SCALE*0.9)
        place_raw("floor-thick", ax, ay, bz + 6*TILE, 0, scale=SCALE*0.6)
    perimeter(cx, cy, 1600, towers=False)

def settle_crash_site(cx, cy):
    # Long debris trail of "wreckage" via tilted floor-thick / wall-low pieces
    for k in range(8):
        dx = -1400 + k*420
        dy = random.uniform(-300, 300)
        dz = ground_z(cx+dx, cy+dy)
        # tilted wall-low chunks = debris
        place_raw("wall-low", cx+dx, cy+dy, dz, random.uniform(0, 360), scale=SCALE*random.uniform(0.6, 1.3))
    # Main fuselage: a long building with no walls
    build_building(cx, cy, 6, 2, ruined=True, door_side=0)
    # Smoke columns (use tall thin columns as smoke poles)
    for _ in range(5):
        x = cx + random.uniform(-1500, 1500); y = cy + random.uniform(-800, 800)
        place_raw("column", x, y, ground_z(x,y), 0, scale=SCALE*0.4)
    crates(cx, cy, 1200, 6)

def settle_fuel_depot(cx, cy):
    # Central command + 4 cylindrical fuel tanks at the cardinal points
    build_building(cx, cy, 3, 3, door_side=0)
    for ad in (0, 90, 180, 270):
        a = math.radians(ad); tx = cx + math.cos(a)*1100; ty = cy + math.sin(a)*1100
        bz = ground_z(tx, ty)
        # Tank = 3 stacked columns with a floor cap (improvised fuel tank look)
        for hi in range(3):
            place_raw("column", tx, ty, bz + hi*TILE, 0, scale=SCALE*1.8)
        place_raw("floor-thick", tx, ty, bz + 3*TILE, 0, scale=SCALE*2.2)
    perimeter(cx, cy, 1800, towers=True)
    crates(cx, cy, 900, 6)

def settle_memorial(cx, cy):
    # Open plaza with column "monument" rows
    floor_z = ground_z(cx, cy)
    # 5x5 plaza floor
    for i in range(-2, 3):
        for j in range(-2, 3):
            place_raw("floor-thick", cx + i*TILE, cy + j*TILE, floor_z, 0)
    # Central tall pillar
    for hi in range(5):
        place_raw("column", cx, cy, floor_z + hi*TILE, 0, scale=SCALE*1.4)
    # Two rows of memorial markers (small columns) flanking the central pillar
    for i in range(-3, 4):
        if i == 0: continue
        place_raw("column", cx + i*TILE*0.8, cy - TILE*2, floor_z, 0, scale=SCALE*0.6)
        place_raw("column", cx + i*TILE*0.8, cy + TILE*2, floor_z, 0, scale=SCALE*0.6)

BUILDERS = {
    "MainBase": settle_main_base, "ForwardOutpost": settle_outpost,
    "RuinedTown": settle_ruined_town, "ResearchStation": settle_research,
    "Checkpoint": settle_checkpoint, "MountainBunker": settle_bunker,
    "Quarry": settle_quarry, "CommArray": settle_comm_array,
    "CrashSite": settle_crash_site, "FuelDepot": settle_fuel_depot,
    "Memorial": settle_memorial,
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
# Scatter scales with world area — bigger map needs more vegetation density to feel alive
VEG_COUNT = int((HALF/25000)**2 * 400) + 400   # ~1400 on 800m map, ~400 on 500m
LAKES = _TH.get("lakes", []) if _TH else []
def in_lake(x, y):
    for L in LAKES:
        if math.hypot(x-L["x"], y-L["y"]) < L["r"]*0.85:
            return True
    return False
# Use nature-kit's variety if it imported, otherwise fall back to tower-defense pieces
have_nature = mesh_cache.get("tree_oak") is not None
for _ in range(VEG_COUNT):
    x = random.uniform(-HALF*0.92, HALF*0.92); y = random.uniform(-HALF*0.92, HALF*0.92)
    if near_poi(x, y) or in_lake(x, y): continue
    roll = random.random()
    if roll < 0.60:
        if have_nature:
            t = random.choice(NATURE_TREES)
            place(t, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.2, 2.4))
        else:
            t = "detail-tree-large" if random.random()<0.5 else "detail-tree"
            place(t, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.5, 3.0))
    elif roll < 0.88:
        if have_nature:
            r = random.choice(NATURE_ROCKS_LARGE if random.random()<0.4 else NATURE_ROCKS_SMALL)
            place(r, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.4, 2.8))
        else:
            r = "detail-rocks-large" if random.random()<0.4 else "detail-rocks"
            place(r, x, y, 0, random.uniform(0,360), scale=SCALE*random.uniform(1.8, 3.5))

level_sub.save_current_level()
unreal.log("══════════════════════════════════════════")
unreal.log(f"OPEN-WORLD BUILD COMPLETE — {placed} objects, {len(POIS)} settlements")
unreal.log("══════════════════════════════════════════")
