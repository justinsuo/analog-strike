"""Post-process raw SceneCapture PNGs: force opaque alpha + copy to claude/snapshot/screenshots/.
Called by tools/claude/snapshot.sh after the editor exits."""
import os, sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RAW = os.path.join(ROOT, "claude", "snapshot", "raw_shots")
OUT = os.path.join(ROOT, "claude", "snapshot", "screenshots")
os.makedirs(OUT, exist_ok=True)

if not os.path.isdir(RAW):
    print(f"No raw_shots/ at {RAW}"); sys.exit(0)

processed = 0
for f in sorted(os.listdir(RAW)):
    if not f.endswith(".png"): continue
    src = os.path.join(RAW, f)
    try:
        im = Image.open(src).convert("RGBA")
        r, g, b, _ = im.split()
        opaque_a = Image.new("L", im.size, 255)
        Image.merge("RGBA", (r, g, b, opaque_a)).convert("RGB").save(os.path.join(OUT, f))
        processed += 1
    except Exception as e:
        print(f"  skip {f}: {e}")

print(f"Post-processed {processed} screenshots -> {OUT}")
