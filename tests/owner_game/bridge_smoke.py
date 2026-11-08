#!/usr/bin/env python3
"""End-to-end Owner Game read/persistence smoke for the *native* Bosman executable.

Start a brand-new Arsenal 2001 career using the existing scripted UI fixture,
advance until a genuine native .sav exists, then restart the executable and load
that .sav. On BOTH game sessions the live owner bridge must identify Arsenal and
read a plausible club balance. All paths live in temporary directories: no real
player saves, sidecars, or settings are touched.

Usage:
    python3 tests/owner_game/bridge_smoke.py /path/to/cm0102_recomp /path/to/game

This deliberately does NOT certify owner funding, loan accounting, contract
persistence, match delegation or atomic cross-domain saves. Those have their own
release gates. It fails the Mac build if even the baseline gameplay bridge fails.
"""
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "fixtures"))
sys.path.insert(0, str(ROOT / "smoke"))

import fixtures  # noqa: E402
import smoke_run  # noqa: E402


def snapshot(user: Path, label: str) -> dict:
    f = user / "owner-game-state.json"
    if not f.is_file():
        raise AssertionError(f"{label}: Owner Game did not write {f.name}")
    state = json.loads(f.read_text(encoding="utf-8"))
    if state.get("ready") is not True:
        raise AssertionError(f"{label}: bridge not ready: {state}")
    if state.get("club_name") != "Arsenal":
        raise AssertionError(f"{label}: unexpected club: {state.get('club_name')!r}")
    if not isinstance(state.get("club_id"), int) or state["club_id"] < 0:
        raise AssertionError(f"{label}: invalid club ID: {state}")
    if not isinstance(state.get("balance"), int) or state["balance"] <= 0:
        raise AssertionError(f"{label}: invalid native club cash: {state}")
    if not isinstance(state.get("date"), str) or len(state["date"]) != 10:
        raise AssertionError(f"{label}: missing game date: {state}")
    if not isinstance(state.get("session"), str) or len(state["session"]) != 16:
        raise AssertionError(f"{label}: invalid process session token: {state}")
    print(f"PASS {label}: Arsenal, club ID {state['club_id']}, "
          f"date {state['date']}, native cash {state['balance']}")
    return state


def run_game(program: str, game: str, user: Path, steps: list[str],
             extra: dict[str, str], stage: str, timeout: int = 900) -> None:
    user.mkdir(parents=True, exist_ok=True)
    fixtures.write_settings(str(user), "classic", [])
    result, took = fixtures.run(program, game, "classic", str(user),
                                smoke_run.scaled_steps("classic", str(user), steps),
                                extra, timeout)
    if result.returncode != 0:
        raise AssertionError(f"{stage}: exited {result.returncode} after "
                             f"{took:.1f}s\n{result.stderr[-4000:]}")
    print(f"PASS {stage}: native game exited cleanly after {took:.1f}s")
    if "recomp: fatal" in result.stderr.lower():
        raise AssertionError(f"{stage}: native game fatal error\n{result.stderr[-2000:]}")


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, argv[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        print("ERROR: pass a compiled native executable and its packaged game directory")
        return 2

    with tempfile.TemporaryDirectory(prefix="owner-game-mac-smoke-") as folder:
        root = Path(folder)
        first = root / "first" / "user"
        second = root / "second" / "user"

        # Existing Bosman first-start script through Arsenal's first-news screen,
        # then choose the *native* holiday command so the game advances > 1 day
        # and writes samesave.sav using its own save function.
        create_steps = [s for s in smoke_run.STEPS if " dump " not in s]
        create_steps = create_steps[:create_steps.index("wait first-news") + 1]
        create_steps += [
            "2500 click 45 165",
            "4500 click 150 427",
            "7000 click 705 572",
            "180000 quit",
        ]
        run_game(program, game, first, create_steps,
                 {"RECOMP_SAVE_AFTER_DAYS": "1"}, "create Arsenal and save")
        original = snapshot(first, "fresh career")
        saved = first / smoke_run.SAVE_NAME
        if not saved.is_file() or saved.stat().st_size < 1024:
            raise AssertionError(f"No native Arsenal .sav created: {saved}")
        print(f"PASS native career saved: {saved.stat().st_size} bytes")

        # A second *process* and a second user directory must be able to load
        # the very same .sav and recreate owner state without any old bridge
        # JSON or queued command carried across.
        second.mkdir(parents=True)
        shutil.copy2(saved, second / smoke_run.SAVE_NAME)
        load_steps = fixtures.LOAD_STEPS + ["3000 quit"]
        run_game(program, game, second, load_steps, {}, "reload Arsenal save")
        restored = snapshot(second, "reloaded career")
        if restored["club_id"] != original["club_id"]:
            raise AssertionError("Native club ID changed after reloading the save")
        if restored["balance"] != original["balance"]:
            raise AssertionError("Native club cash changed across save/reload with no transaction")
        if restored["session"] == original["session"]:
            raise AssertionError("Process session token unexpectedly survived restart")
        if (second / "owner-game-command.txt").exists():
            raise AssertionError("Stale owner command appeared in reloaded career")
        print("PASS owner bridge reboot/readback: stable club and cash, no stale command")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError) as error:
        print(f"FAIL Owner Game Mac bridge smoke: {error}", file=sys.stderr)
        sys.exit(1)
