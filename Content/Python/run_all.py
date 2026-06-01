"""
MASTER SETUP SCRIPT - Run this once inside UE5 editor to set up everything.
py /tmp/ue_run_all.py
"""
import unreal
import subprocess
import os

scripts = [
    "/tmp/ue_fix_character.py",
    "/tmp/ue_populate_world.py",
]

for script in scripts:
    if os.path.exists(script):
        unreal.log(f"Running: {script}")
        exec(open(script).read())
    else:
        unreal.log_warning(f"Script not found: {script}")

unreal.log("═══════════════════════════════════════")
unreal.log("ALL SETUP SCRIPTS COMPLETE!")
unreal.log("Press Play to test the game.")
unreal.log("═══════════════════════════════════════")
