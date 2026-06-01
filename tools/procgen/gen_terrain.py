"""
ANALOG STRIKE — OPEN-WORLD TERRAIN GENERATOR
Builds a large, varied landscape (~500 m) with:
  - rolling hills (fractal noise)
  - a mountain range along the north edge
  - a lake basin in the south-east
  - flat building pads carved at each settlement (so structures sit on flat ground)
Exports:
  ~/Downloads/as_terrain.obj           walkable terrain static mesh
  ~/Downloads/as_heightmap.png         16-bit heightmap
  ~/Downloads/as_terrain_heights.json  height grid + POI list for the build script
Pure local Python (numpy + PIL).
"""
import numpy as np
from PIL import Image
import json, os

N = 320                 # grid resolution
WORLD = 50000.0         # 500 m across
MAX_HEIGHT = 9000.0     # tallest peaks
WATER_Z = 0.10          # normalized lake floor level
np.random.seed(21)

# Points of interest: (name, world_x, world_y, pad_radius_uu)
POIS = [
    ("MainBase",      0,      0,     5000),
    ("ForwardOutpost", -9000,  9000,  3000),
    ("RuinedTown",     11000,  3000,  4000),
    ("ResearchStation", -11000, -5000, 3000),
    ("Checkpoint",     2000,  -11000, 2600),
    ("MountainBunker", 7000,   13000, 2400),
]

def smooth_upsample(grid, n):
    g = np.asarray(grid, float); src = g.shape[0]
    xs = np.linspace(0, src-1, n); tmp = np.empty((src, n)); cols = np.arange(src)
    for i in range(src): tmp[i] = np.interp(xs, cols, g[i])
    out = np.empty((n, n)); rows = np.arange(src)
    for j in range(n): out[:, j] = np.interp(xs, rows, tmp[:, j])
    return out

def fractal(n, octaves, seed=0):
    rng = np.random.RandomState(seed); h = np.zeros((n, n)); amp=1.0; tot=0.0; res=2
    for _ in range(octaves):
        h += smooth_upsample(rng.rand(res, res), n) * amp; tot+=amp; amp*=0.5; res*=2
    return h/tot

# Coordinate grids
jj, ii = np.mgrid[0:N, 0:N]
def gx(i): return -WORLD/2 + i/(N-1)*WORLD
def gy(j): return -WORLD/2 + j/(N-1)*WORLD
WX = -WORLD/2 + ii/(N-1)*WORLD
WY = -WORLD/2 + jj/(N-1)*WORLD

# Base rolling hills (more amplitude for a varied interior)
base = fractal(N, 7, seed=1)
ridged = 1.0 - np.abs(fractal(N, 5, seed=2) * 2 - 1)
ridged2 = 1.0 - np.abs(fractal(N, 6, seed=3) * 2 - 1)

# Mountain rim around the whole map — natural open-world boundary
r = np.sqrt(WX**2 + WY**2) / (WORLD/2)      # 0 center, ~1 edge, >1 corners
rim = np.clip((r - 0.60) / 0.40, 0, 1) ** 1.5
rim_mountains = rim * (0.70 + 0.55 * ridged)

# Taller dramatic range along the north (high +Y)
ny = (WY / (WORLD/2))
north = np.clip((ny - 0.15) / 0.85, 0, 1) ** 1.3
north_range = north * (0.55 + 0.55 * ridged2)

# Combine: rolling interior hills + rim + north range
h = base * 0.32 + rim_mountains * 1.0 + north_range * 0.75
h -= h.min(); h /= h.max()

# Lake basin (kept in the interior, not up in the rim)
lake_x, lake_y, lake_r = 9500.0, -8000.0, 5500.0
ld = np.sqrt((WX - lake_x)**2 + (WY - lake_y)**2)
lake_mask = np.clip(1 - ld / lake_r, 0, 1) ** 1.5
h = h * (1 - lake_mask) + np.minimum(h, WATER_Z) * lake_mask
h = np.maximum(h, np.where(lake_mask > 0.5, WATER_Z * 0.6, 0))

# Flatten building pads at each POI
for (_, wxp, wyp, rad) in POIS:
    d = np.sqrt((WX - wxp)**2 + (WY - wyp)**2)
    m = np.clip(1 - d / rad, 0, 1)
    m = m * m * (3 - 2 * m)                 # smoothstep
    # local target height = value at the pad center
    ci = int((wxp + WORLD/2)/WORLD*(N-1)); cj = int((wyp + WORLD/2)/WORLD*(N-1))
    ci = min(max(ci,0),N-1); cj = min(max(cj,0),N-1)
    target = h[cj, ci]
    h = h * (1 - m) + target * m

h = np.clip(h, 0, 1)

# ── Heightmap PNG ──
out_png = os.path.expanduser("~/Downloads/as_heightmap.png")
Image.fromarray((h*65535).astype(np.uint16)).save(out_png)
print(f"Heightmap: {out_png} ({N}x{N})")

# ── Terrain OBJ ──
out_obj = os.path.expanduser("~/Downloads/as_terrain.obj")
step = WORLD/(N-1); half = WORLD/2
with open(out_obj, "w") as f:
    f.write("# Analog Strike open-world terrain\n")
    for j in range(N):
        for i in range(N):
            f.write(f"v {gx(i):.1f} {gy(j):.1f} {h[j,i]*MAX_HEIGHT:.1f}\n")
    tile=40.0
    for j in range(N):
        for i in range(N):
            f.write(f"vt {i/(N-1)*tile:.3f} {j/(N-1)*tile:.3f}\n")
    def idx(i,j): return j*N+i+1
    for j in range(N-1):
        for i in range(N-1):
            a,b,c,d=idx(i,j),idx(i+1,j),idx(i+1,j+1),idx(i,j+1)
            f.write(f"f {a}/{a} {b}/{b} {c}/{c}\n"); f.write(f"f {a}/{a} {c}/{c} {d}/{d}\n")
print(f"Terrain OBJ: {out_obj}  ({N*N} verts, {(N-1)*(N-1)*2} tris, {os.path.getsize(out_obj)/1e6:.1f} MB)")

# ── Coarse height grid + POIs as JSON ──
COARSE=96
ys=np.linspace(0,N-1,COARSE).astype(int); xs=np.linspace(0,N-1,COARSE).astype(int)
grid=[[float(h[j,i]*MAX_HEIGHT) for i in xs] for j in ys]
out_json=os.path.expanduser("~/Downloads/as_terrain_heights.json")
with open(out_json,"w") as jf:
    json.dump({"n":COARSE,"world":WORLD,"max_height":MAX_HEIGHT,"grid":grid,
               "pois":[{"name":n,"x":x,"y":y,"r":r} for (n,x,y,r) in POIS],
               "water_z":WATER_Z*MAX_HEIGHT,
               "lake":{"x":lake_x,"y":lake_y,"r":lake_r}}, jf)
print(f"Height+POI grid: {out_json} ({COARSE}x{COARSE})")
print(f"World {WORLD:.0f}uu, peaks {MAX_HEIGHT:.0f}uu, {len(POIS)} settlements")
