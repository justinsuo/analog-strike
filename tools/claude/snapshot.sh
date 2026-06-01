#!/bin/bash
# Analog Strike — Claude snapshot pipeline
# Runs UE5 headless to dump level state + screenshots, then post-processes them
# into claude/snapshot/ so Claude (or you) can review the current game state.
#
# Usage:  ./tools/claude/snapshot.sh        (from the project root)
# Requires: editor closed (don't run while the editor is open — file locks).

set -e

PROJ_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"
cd "$PROJ_DIR"

UE="/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor-Cmd"
UPROJECT="$PROJ_DIR/AnalogStrike.uproject"

if [ ! -x "$UE" ]; then
    echo "ERROR: UE5 not found at $UE — edit this script's UE variable."
    exit 1
fi
if ! pgrep -fl "UnrealEditor" > /dev/null 2>&1; then
    :   # editor not running, good
else
    echo "WARNING: an UnrealEditor process is already running. File locks may cause failures."
    echo "         Close the editor first for best results."
    sleep 2
fi

mkdir -p claude/snapshot/raw_shots claude/snapshot/screenshots

echo "[1/3] Running UE5 headless snapshot (this takes a few minutes — editor boots)..."
"$UE" "$UPROJECT" \
    -ExecutePythonScript="$PROJ_DIR/tools/claude/snapshot.py" \
    -RenderOffScreen -nosplash -unattended -stdout \
    > claude/snapshot/_editor_run.log 2>&1 || true

echo "[2/3] Post-processing screenshots (force opaque alpha)..."
python3 "$PROJ_DIR/tools/claude/postprocess.py"

echo "[3/3] Rendering top-down layout map from state.json..."
python3 "$PROJ_DIR/tools/claude/render_layout.py"

echo ""
echo "═══════════════════════════════════════"
echo "  SNAPSHOT READY — claude/snapshot/"
echo "═══════════════════════════════════════"
ls -la claude/snapshot/ 2>/dev/null
echo ""
if [ -f claude/snapshot/state.json ]; then
    echo "Quick summary:"
    python3 -c "
import json
d=json.load(open('claude/snapshot/state.json'))
print(f'  Level:           {d[\"level\"]}')
print(f'  Timestamp:       {d[\"timestamp\"]}')
print(f'  Total actors:    {d[\"stats\"][\"total_actors\"]}')
print(f'  Terrain present: {d[\"stats\"][\"terrain_present\"]}')
print(f'  Top actor types:')
for cls, n in sorted(d['actor_summary'].items(), key=lambda kv:-kv[1])[:6]:
    print(f'    {n:>4} × {cls}')
"
fi
echo ""
echo "Tell Claude:  'I ran snapshot' — they will read claude/snapshot/ and respond."
