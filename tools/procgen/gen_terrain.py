"""
ANALOG STRIKE — OPEN-WORLD TERRAIN GENERATOR v3 (organic, vast)
The world no longer feels like a square with a bowl. Continuous fractal
landscape — mountain ranges, ridges, valleys, and lowlands extend organically
in every direction. The 11 settlements are flat pads carved into this wider
wilderness; no artificial circular boundary.

Outputs:
  ~/Downloads/as_terrain.obj                walkable static mesh terrain
  ~/Downloads/as_heightmap.png              16-bit grayscale heightmap
  ~/Downloads/as_terrain_heights.json       coarse grid + POI + water info
"""
import numpy as np
from PIL import Image
import json, os, math

N = 512                  # heightmap resolution
WORLD = 150000.0         # 1500 m across (was 800)
MAX_HEIGHT = 18000.0     # 180 m peaks
WATER_Z = 0.18           # absolute (normalised) lake/river water level
np.random.seed(99)

# Settlements — spread across a wider area now that the world is much bigger.
# Distances roughly doubled from v2 to fill more of the wilderness.
POIS = [
    ("MainBase",            0,       0,    6000),
    ("ForwardOutpost",    -28000,  30000,  3400),
    ("RuinedTown",         34000,   8000,  4800),
    ("ResearchStation",   -34000, -15000,  3400),
    ("Checkpoint",          8000, -34000,  3000),
    ("MountainBunker",     22000,  40000,  2600),
    ("Quarry",             42000, -18000,  3200),
    ("CommArray",         -40000,  12000,  2600),
    ("CrashSite",          14000, -14000,  2400),
    ("FuelDepot",         -14000, -30000,  2800),
    ("Memorial",            0,    -18000,  2200),
]

def smooth_upsample(grid, n):
    g = np.asarray(grid, float); src = g.shape[0]
    xs = np.linspace(0, src-1, n); tmp = np.empty((src, n)); cols = np.arange(src)
    for i in range(src): tmp[i] = np.interp(xs, cols, g[i])
    out = np.empty((n, n)); rows = np.arange(src)
    for j in range(n): out[:, j] = np.interp(xs, rows, tmp[:, j])
    return out

def fractal(n, octaves, seed=0, base_res=2):
    rng = np.random.RandomState(seed); h = np.zeros((n,n)); amp=1.0; tot=0.0; res=base_res
    for _ in range(octaves):
        h += smooth_upsample(rng.rand(res,res), n) * amp
        tot += amp; amp *= 0.5; res *= 2
    return h/tot

jj, ii = np.mgrid[0:N, 0:N]

# ── Layered organic terrain — no radial bowl ──
# Big-wavelength "biome amplitude": some regions inherently mountainous,
# others lowland. No artificial boundary; just very-low-frequency noise.
biome = fractal(N, 4, seed=11, base_res=2)
biome = (biome - biome.min()) / (biome.max() - biome.min())
biome = biome ** 1.3                                # exaggerate mountain regions

# Medium-frequency rolling base (rolling hills)
base = fractal(N, 9, seed=21, base_res=3)

# Two layers of ridged noise — sharper mountain ridges
ridged_a = 1.0 - np.abs(fractal(N, 7, seed=31, base_res=2) * 2 - 1)
ridged_b = 1.0 - np.abs(fractal(N, 6, seed=41, base_res=4) * 2 - 1)
ridged = (ridged_a * 0.6 + ridged_b * 0.4) ** 1.4

# Combine: biome controls how mountainous a region is.
# Where biome ~ 1: strong mountain ridges + base
# Where biome ~ 0: gentle rolling base only
h = base * 0.35 + ridged * biome * 0.85

# Add a long-wavelength "valley network" — subtle horizontal/diagonal valleys
valley_noise = fractal(N, 3, seed=51, base_res=3)
valley = 1.0 - np.abs(valley_noise * 2 - 1)
h = h - valley * 0.06

# Normalize, raise floor a bit so we have headroom for water carving
h -= h.min(); h /= h.max()

# Coordinate grids in world units
WX = -WORLD/2 + ii/(N-1)*WORLD
WY = -WORLD/2 + jj/(N-1)*WORLD

# ── River — winds NW -> SE through the central area ──
river_pts = [
    (-65000, 45000), (-40000, 30000), (-18000, 15000),
    (-4000,  4000), (10000, -8000), (28000, -22000), (55000, -48000),
]
def line_dist(pts):
    px = WX.flatten(); py = WY.flatten()
    minD = np.full_like(px, 1e9)
    for k in range(len(pts)-1):
        ax,ay = pts[k]; bx,by = pts[k+1]
        ex,ey = bx-ax, by-ay; L2 = ex*ex + ey*ey
        t = np.clip(((px-ax)*ex + (py-ay)*ey) / L2, 0, 1)
        cx,cy = ax+t*ex, ay+t*ey
        minD = np.minimum(minD, np.sqrt((px-cx)**2 + (py-cy)**2))
    return minD.reshape(WX.shape)
riv_d = line_dist(river_pts)
river_m = np.exp(-(riv_d / 2200.0) ** 2)
h = np.clip(h - river_m * 0.18, 0, 1)

# ── Lakes — multiple natural-looking basins ──
lakes = [
    ("LakeNorth",  -18000,  44000, 8000, 0.20),
    ("LakeSouth",   30000, -50000, 9500, 0.22),
    ("LakeEast",    52000,  18000, 6500, 0.18),  # NEW (more lakes for bigger world)
]
for (_, lx, ly, lr, ld) in lakes:
    d = np.sqrt((WX - lx)**2 + (WY - ly)**2)
    m = np.clip(1 - d / lr, 0, 1) ** 1.5
    h = np.clip(h - m * ld, 0, 1)
    floor = np.clip(1 - d / lr, 0, 1) ** 2
    h = np.where(floor > 0.05, np.minimum(h, WATER_Z * 0.7), h)

# ── Two canyons (carved deep narrow notches) ──
canyon_a = [(26000, 12000), (28000, 0), (30000, -14000)]
canyon_b = [(-44000, -30000), (-38000, -38000), (-30000, -46000)]
for pts in (canyon_a, canyon_b):
    can_d = line_dist(pts)
    can_m = np.exp(-(can_d / 1600.0) ** 2)
    h = np.clip(h - can_m * 0.22, 0, 1)

# ── Settlement pads — smoothstep flatten ──
for (_, wxp, wyp, rad) in POIS:
    d = np.sqrt((WX - wxp)**2 + (WY - wyp)**2)
    m = np.clip(1 - d / rad, 0, 1)
    m = m * m * (3 - 2*m)
    ci = int((wxp + WORLD/2)/WORLD*(N-1)); cj = int((wyp + WORLD/2)/WORLD*(N-1))
    ci = min(max(ci, 0), N-1); cj = min(max(cj, 0), N-1)
    target = h[cj, ci]
    h = h * (1 - m) + target * m

h = np.clip(h, 0, 1)

# ── Heightmap PNG ──
out_png = os.path.expanduser("~/Downloads/as_heightmap.png")
Image.fromarray((h * 65535).astype(np.uint16)).save(out_png)
print(f"Heightmap PNG: {out_png}  ({N}x{N})")

# ── Terrain OBJ ──
out_obj = os.path.expanduser("~/Downloads/as_terrain.obj")
step = WORLD / (N - 1); half = WORLD / 2
with open(out_obj, "w") as f:
    f.write("# Analog Strike open-world terrain v3 (organic, vast)\n")
    for j in range(N):
        for i in range(N):
            x = -half + i*step; y = -half + j*step
            f.write(f"v {x:.1f} {y:.1f} {h[j,i]*MAX_HEIGHT:.1f}\n")
    tile = 160.0
    for j in range(N):
        for i in range(N):
            f.write(f"vt {i/(N-1)*tile:.3f} {j/(N-1)*tile:.3f}\n")
    def idx(i, j): return j*N + i + 1
    for j in range(N-1):
        for i in range(N-1):
            a,b,c,d = idx(i,j), idx(i+1,j), idx(i+1,j+1), idx(i,j+1)
            f.write(f"f {a}/{a} {b}/{b} {c}/{c}\n")
            f.write(f"f {a}/{a} {c}/{c} {d}/{d}\n")
print(f"Terrain OBJ: {out_obj}  ({N*N} verts, {(N-1)*(N-1)*2} tris, {os.path.getsize(out_obj)/1e6:.1f} MB)")

# ── Coarse grid + POI + water JSON ──
COARSE = 160
ys = np.linspace(0, N-1, COARSE).astype(int); xs = np.linspace(0, N-1, COARSE).astype(int)
grid = [[float(h[j,i] * MAX_HEIGHT) for i in xs] for j in ys]
data = {
    "n": COARSE, "world": WORLD, "max_height": MAX_HEIGHT,
    "grid": grid,
    "pois": [{"name": n, "x": x, "y": y, "r": r} for (n, x, y, r) in POIS],
    "water_z": WATER_Z * MAX_HEIGHT,
    "lakes": [{"name": n, "x": x, "y": y, "r": r} for (n, x, y, r, _) in lakes],
    "river": river_pts,
    "canyons": [canyon_a, canyon_b],
}
out_json = os.path.expanduser("~/Downloads/as_terrain_heights.json")
with open(out_json, "w") as jf:
    json.dump(data, jf)
print(f"Height + POI + water JSON: {out_json}  ({COARSE}x{COARSE} samples)")
print(f"World {WORLD:.0f}uu ({WORLD/100:.0f}m), peaks {MAX_HEIGHT:.0f}uu, {len(POIS)} settlements, {len(lakes)} lakes, river, 2 canyons")
