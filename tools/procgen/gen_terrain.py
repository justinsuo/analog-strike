"""
ANALOG STRIKE — OPEN-WORLD TERRAIN GENERATOR v2 (big & comprehensive)
A larger, more varied landscape (~800 m):
  - bowl-shaped basin ringed by mountains (boundary)
  - taller northern mountain range with a glacial peak
  - a river running NW -> SE through the basin
  - two lakes (north + south)
  - a sunken canyon east of center
  - 11 distinct settlements (POIs) with flat building pads
Exports:
  ~/Downloads/as_terrain.obj                  walkable static-mesh terrain
  ~/Downloads/as_heightmap.png                16-bit grayscale heightmap
  ~/Downloads/as_terrain_heights.json         coarse grid + POI list + water info
"""
import numpy as np
from PIL import Image
import json, os, math

N = 384
WORLD = 80000.0          # 800 m across (was 500)
MAX_HEIGHT = 12000.0     # taller peaks
WATER_Z = 0.10
np.random.seed(42)

# 11 settlements
POIS = [
    ("MainBase",           0,      0,     5500),
    ("ForwardOutpost",    -15000,  16000, 3200),
    ("RuinedTown",         18000,  4000,  4500),
    ("ResearchStation",   -18000, -8000,  3200),
    ("Checkpoint",         4000,  -18000, 2800),
    ("MountainBunker",     12000,  22000, 2400),
    ("Quarry",             22000, -10000, 3000),
    ("CommArray",         -22000,  6000,  2400),
    ("CrashSite",          8000,  -8000,  2200),
    ("FuelDepot",         -8000,  -16000, 2600),
    ("Memorial",           0,     -10000, 2000),
]

def smooth_upsample(grid, n):
    g = np.asarray(grid, float); src = g.shape[0]
    xs = np.linspace(0, src-1, n); tmp = np.empty((src, n)); cols = np.arange(src)
    for i in range(src): tmp[i] = np.interp(xs, cols, g[i])
    out = np.empty((n, n)); rows = np.arange(src)
    for j in range(n): out[:, j] = np.interp(xs, rows, tmp[:, j])
    return out
def fractal(n, octaves, seed=0):
    rng = np.random.RandomState(seed); h = np.zeros((n,n)); amp=1.0; tot=0.0; res=2
    for _ in range(octaves):
        h += smooth_upsample(rng.rand(res,res), n) * amp
        tot += amp; amp *= 0.5; res *= 2
    return h/tot

jj, ii = np.mgrid[0:N, 0:N]
WX = -WORLD/2 + ii/(N-1)*WORLD
WY = -WORLD/2 + jj/(N-1)*WORLD

base = fractal(N, 8, seed=1)
ridged  = 1.0 - np.abs(fractal(N, 6, seed=2) * 2 - 1)
ridged2 = 1.0 - np.abs(fractal(N, 6, seed=3) * 2 - 1)

# Mountain rim around perimeter
r = np.sqrt(WX**2 + WY**2) / (WORLD/2)
rim = np.clip((r - 0.55) / 0.45, 0, 1) ** 1.5
rim_mountains = rim * (0.75 + 0.55 * ridged)

# Tall north range with glacial peak
ny = WY / (WORLD/2)
north = np.clip((ny - 0.10) / 0.90, 0, 1) ** 1.2
glacier_d = np.sqrt((WX - 5000)**2 + (WY - 28000)**2) / 12000
glacier = np.exp(-glacier_d**2) * 0.45
north_range = north * (0.60 + 0.60 * ridged2) + glacier

# SE rocky range
sw = np.clip((WX/(WORLD/2)) - 0.30, 0, 1) * np.clip(-(WY/(WORLD/2)) - 0.05, 0, 1)
se_range = sw ** 1.2 * 0.55 * ridged2

h = base * 0.30 + rim_mountains + north_range * 0.85 + se_range * 0.55
h -= h.min(); h /= h.max()

# River NW -> SE
river_pts = [(-32000, 20000), (-18000, 12000), (-6000, 2000),
             (4000, -2000), (14000, -12000), (26000, -22000)]
def line_dist(pts, WX, WY):
    px = WX.flatten(); py = WY.flatten()
    minD = np.full_like(px, 1e9)
    for k in range(len(pts)-1):
        ax,ay = pts[k]; bx,by = pts[k+1]
        ex,ey = bx-ax, by-ay; L2 = ex*ex + ey*ey
        t = np.clip(((px-ax)*ex + (py-ay)*ey)/L2, 0, 1)
        cx,cy = ax + t*ex, ay + t*ey
        minD = np.minimum(minD, np.sqrt((px-cx)**2 + (py-cy)**2))
    return minD.reshape(WX.shape)
riv_d = line_dist(river_pts, WX, WY)
river_m = np.exp(-(riv_d / 1500.0) ** 2)
h = np.clip(h - river_m * 0.18, 0, 1)

# Lakes
lakes = [
    ("LakeNorth",  -10000,  24000, 6000, 0.20),
    ("LakeSouth",   16000, -28000, 7500, 0.22),
]
for (_, lx, ly, lr, ld) in lakes:
    d = np.sqrt((WX - lx)**2 + (WY - ly)**2)
    m = np.clip(1 - d / lr, 0, 1) ** 1.5
    h = np.clip(h - m * ld, 0, 1)
    lake_floor = np.clip(1 - d / lr, 0, 1) ** 2
    h = np.where(lake_floor > 0.05, np.minimum(h, WATER_Z * 0.7), h)

# Canyon (deep narrow notch east of center)
canyon_pts = [(14000, 6000), (16000, -2000), (18000, -10000)]
can_d = line_dist(canyon_pts, WX, WY)
can_m = np.exp(-(can_d / 1200.0) ** 2)
h = np.clip(h - can_m * 0.25, 0, 1)

# Flatten settlement pads
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
    f.write("# Analog Strike open-world terrain v2\n")
    for j in range(N):
        for i in range(N):
            x = -half + i*step; y = -half + j*step
            f.write(f"v {x:.1f} {y:.1f} {h[j,i]*MAX_HEIGHT:.1f}\n")
    tile = 80.0
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
COARSE = 128
ys = np.linspace(0, N-1, COARSE).astype(int); xs = np.linspace(0, N-1, COARSE).astype(int)
grid = [[float(h[j,i] * MAX_HEIGHT) for i in xs] for j in ys]
data = {
    "n": COARSE, "world": WORLD, "max_height": MAX_HEIGHT,
    "grid": grid,
    "pois": [{"name": n, "x": x, "y": y, "r": r} for (n, x, y, r) in POIS],
    "water_z": WATER_Z * MAX_HEIGHT,
    "lakes": [{"name": n, "x": x, "y": y, "r": r} for (n, x, y, r, _) in lakes],
    "river": river_pts,
    "canyon": canyon_pts,
}
out_json = os.path.expanduser("~/Downloads/as_terrain_heights.json")
with open(out_json, "w") as jf:
    json.dump(data, jf)
print(f"Height + POI + water JSON: {out_json}  ({COARSE}x{COARSE} samples)")
print(f"World {WORLD:.0f}uu ({WORLD/100:.0f}m), peaks {MAX_HEIGHT:.0f}uu, {len(POIS)} settlements, {len(lakes)} lakes, river, canyon")
