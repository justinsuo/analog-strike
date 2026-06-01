"""Master runner: import terrain -> build map -> capture, in one headless session."""
import unreal, traceback

steps = ["/tmp/ue_import_terrain.py", "/tmp/ue_build_map.py", "/tmp/ue_capture.py"]
for s in steps:
    unreal.log(f"===== RUNNING {s} =====")
    try:
        exec(open(s).read(), {"__name__": "__main__"})
        unreal.log(f"===== OK {s} =====")
    except Exception:
        unreal.log_error(f"===== FAILED {s} =====")
        unreal.log_error(traceback.format_exc())
unreal.log("===== MASTER COMPLETE =====")
# Close the editor so the headless run terminates
try:
    unreal.SystemLibrary.quit_editor()
except Exception:
    try:
        unreal.EditorLevelLibrary.editor_request_exit() if hasattr(unreal.EditorLevelLibrary, "editor_request_exit") else None
    except Exception:
        pass
