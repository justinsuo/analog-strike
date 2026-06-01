"""
Local Python: read claude/snapshot/state.json and render a top-down layout map
showing all actor positions colored by class. Works without UE — purely numpy/PIL.
Output: claude/snapshot/layout_top.png  (a deterministic "minimap" Claude can read).
"""
import json, os, sys
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SNAP = os.path.join(ROOT, "claude", "snapshot")
STATE = os.path.join(SNAP, "state.json")
OUT = os.path.join(SNAP, "layout_top.png")

if not os.path.exists(STATE):
    print(f"No state.json at {STATE} — run snapshot.sh first."); sys.exit(1)

with open(STATE) as f: data = json.load(f)
actors = data["actors"]

W, H = 1200, 1200
img = Image.new("RGB", (W, H), (28, 32, 38))
draw = ImageDraw.Draw(img)

# World extent — auto-fit to actor positions (with some padding)
xs = [a["x"] for a in actors] + [-25000, 25000]
ys = [a["y"] for a in actors] + [-25000, 25000]
minx, maxx = min(xs), max(xs)
miny, maxy = min(ys), max(ys)
pad = 1000
minx -= pad; maxx += pad; miny -= pad; maxy += pad
span = max(maxx-minx, maxy-miny)
def sx(x): return int((x - minx) / span * (W - 40)) + 20
def sy(y): return int((y - miny) / span * (H - 40)) + 20

# Grid
for g in range(-25000, 25001, 5000):
    if minx <= g <= maxx:
        draw.line([(sx(g), 20), (sx(g), H-20)], fill=(40, 44, 50))
    if miny <= g <= maxy:
        draw.line([(20, sy(g)), (W-20, sy(g))], fill=(40, 44, 50))

# Color buckets by actor class/label
def color_for(a):
    cls = a["class"]; lbl = (a["label"] or "").lower()
    if "terrain" in lbl: return (60, 110, 60), 2
    if "Light" in cls or "Atmosphere" in cls or "Fog" in cls: return (240, 220, 80), 4
    if "Wall" in (a.get("mesh") or ""): return (160, 130, 100), 2
    if "Floor" in (a.get("mesh") or ""): return (120, 100, 80), 2
    if "Column" in (a.get("mesh") or ""): return (190, 180, 160), 3
    if "tree" in (a.get("mesh") or "").lower(): return (60, 140, 60), 3
    if "rock" in (a.get("mesh") or "").lower(): return (140, 120, 110), 3
    if "crate" in (a.get("mesh") or "").lower(): return (180, 130, 60), 2
    if "crystal" in (a.get("mesh") or "").lower(): return (180, 100, 220), 3
    if "Enemy" in cls or "Security" in cls or "Sniper" in cls or "Warden" in cls: return (230, 60, 50), 4
    if "Pickup" in cls or "Ammo" in cls or "Heal" in cls: return (60, 220, 120), 4
    if "Relay" in cls or "Extraction" in cls: return (80, 200, 240), 5
    return (110, 120, 140), 2

# Draw actors (sorted so big things underneath)
order = {"AS_Terrain": 0}
for a in sorted(actors, key=lambda a: order.get(a.get("label","") , 5)):
    col, r = color_for(a)
    px, py = sx(a["x"]), sy(a["y"])
    if 0 <= px < W and 0 <= py < H:
        draw.ellipse([px-r, py-r, px+r, py+r], fill=col)

# Title + legend + summary
draw.text((20, 4), f"Analog Strike — Layout top-down ({data['timestamp']})", fill=(220, 220, 220))
leg_y = H - 130
legend = [
    ("Terrain", (60,110,60)), ("Walls/Floors", (160,130,100)),
    ("Trees", (60,140,60)), ("Rocks", (140,120,110)),
    ("Crates", (180,130,60)), ("Enemies", (230,60,50)),
    ("Pickups", (60,220,120)), ("Objectives", (80,200,240)),
    ("Lights", (240,220,80)),
]
for i, (name, c) in enumerate(legend):
    y = leg_y + (i//3)*22
    x = 20 + (i%3)*200
    draw.ellipse([x, y, x+10, y+10], fill=c)
    draw.text((x+18, y-2), name, fill=(210,210,210))

# Summary text top right
summary = data.get("actor_summary", {})
top = sorted(summary.items(), key=lambda kv: -kv[1])[:8]
sy_y = 30
draw.text((W-280, sy_y), f"Total actors: {data['stats']['total_actors']}", fill=(220, 220, 220)); sy_y += 18
for name, n in top:
    draw.text((W-280, sy_y), f"  {n:>4} × {name}", fill=(180, 180, 180)); sy_y += 16

img.save(OUT)
print(f"Layout rendered: {OUT}")
