#!/usr/bin/env python3
"""Run a real native CM manager week in an isolated career, never invent results."""
from datetime import date
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "fixtures"))
sys.path.insert(0, str(HERE.parent / "smoke"))
import fixtures
import smoke_run


def snapshot(p):
    try:
        return json.loads(p.read_text())
    except (OSError, ValueError):
        return {}


def run(program, game, root):
    user = root / "user"
    user.mkdir()
    (root / "home").mkdir()
    fixtures.write_settings(str(user), "classic", [
        ("faster_days", "on"), ("changing_rules", "off")
    ])
    steps = [s for s in smoke_run.STEPS if " dump " not in s]
    steps = steps[:steps.index("wait first-news") + 1]
    # Use the same native Continue location as the proven arm/cancel test.
    # No input once the native holiday begins (the test exits on completion).
    steps += ["12000 click 45 115", "29000 click 45 115",
              "46000 click 45 115", "63000 click 45 115",
              "180000 quit"]
    env = dict(os.environ, HOME=str(root / "home"),
               SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
               RECOMP_NO_BOX="1", RECOMP_USER_DIR=str(user),
               RECOMP_FIXED_CLOCK="1",
               RECOMP_SCRIPT="; ".join(smoke_run.scaled_steps("classic", str(user), steps)))
    state_path = user / "owner-game-state.json"
    result_path = user / "owner-game-result.json"
    cmd = user / "owner-game-command.txt"
    log_path = root / "native.log"
    sent = applied = False
    first = last = None
    phases = set()
    started = time.monotonic()
    with log_path.open("wb") as log:
        proc = subprocess.Popen([program, "--game", game, "-seed", "1"],
                                env=env, stdout=subprocess.DEVNULL, stderr=log)
        try:
            while proc.poll() is None and time.monotonic() - started < 240:
                s = snapshot(state_path)
                if s.get("ready"):
                    assert s["club_name"] == "Arsenal"
                    if not sent:
                        cmd.write_text(f"delegate_week {s['session']} {s['club_id']}\n")
                        sent = True
                    r = snapshot(result_path)
                    if r.get("message") == "native manager week started":
                        assert r["status"] == "applied"
                        assert r["before"] == r["after"]
                        applied = True
                    phase = s.get("manager_week_status", 0)
                    phases.add(phase)
                    if phase == 1 and first is None:
                        first = date.fromisoformat(s["date"])
                    if phase == 2:
                        last = date.fromisoformat(s["date"])
                        break
                time.sleep(.1)
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=8)
    log = log_path.read_text(errors="replace")
    assert sent and applied and first and last, f"week incomplete: phases={phases}\n{log[-4000:]}"
    assert (last-first).days >= 7, f"week ended prematurely: {first} -> {last}"
    assert log.count("manager week started") == 1
    assert log.count("manager week complete; control returned") == 1
    assert not cmd.exists()
    print(f"PASS native manager week: {first} -> {last}, control returned")


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="owner-game-week-") as tmp:
        run(os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2]), Path(tmp))
