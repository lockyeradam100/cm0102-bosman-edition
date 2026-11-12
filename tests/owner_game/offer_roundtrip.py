#!/usr/bin/env python3
"""Owner Game P1 native contract save/reload test (isolated Mac test career).

The *native* engine must create at least one contract offer while Arsenal's
manager is holidaying, save the career, and reload it in a distinct process.
The unloaded/reloaded state must have the exact same CM game date, human club,
and decoded contract.dat bytes.

This is a real native offer + save/reload integrity gate. It does NOT yet
certify the separate GUI-submitted Figo offer or acceptance/rejection outcome.
Nothing is injected into a player's contract and no personal saves are used.

Usage:
    python3 tests/owner_game/offer_roundtrip.py <cm0102_recomp> <game directory>
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "fixtures"))
sys.path.insert(0, str(ROOT / "smoke"))
sys.path.insert(0, str(ROOT.parent / "tools" / "save"))

import fixtures  # noqa: E402
import smoke_run  # noqa: E402
from savfile import Sav  # noqa: E402

ADD = re.compile(r"owner-game: contract native_add_called staff=(\d+) club=(\d+)")
DAILY = re.compile(r"owner-game: contract native_daily_pass staff=(\d+) club=(\d+)")


def run_native(program: str, game: str, user: Path, steps: list[str],
               env: dict[str, str], stage: str) -> str:
    user.mkdir(parents=True, exist_ok=True)
    fixtures.write_settings(str(user), "classic", [
        ("faster_days", "on"), ("changing_rules", "off")
    ])
    result, elapsed = fixtures.run(
        program, game, "classic", str(user),
        smoke_run.scaled_steps("classic", str(user), steps),
        env, timeout=900
    )
    if result.returncode:
        raise AssertionError(
            f"{stage}: process exited {result.returncode} in {elapsed:.1f}s\n"
            + result.stderr[-3500:]
        )
    if "recomp: saved" not in result.stderr:
        raise AssertionError(f"{stage}: no native save confirmation\n"
                             + result.stderr[-3500:])
    return result.stderr


def owner_state(user: Path, stage: str) -> dict:
    path = user / "owner-game-state.json"
    if not path.is_file():
        raise AssertionError(f"{stage}: no owner bridge state")
    state = json.loads(path.read_text(encoding="utf-8"))
    if state.get("ready") is not True or state.get("club_name") != "Arsenal":
        raise AssertionError(f"{stage}: game not an active Arsenal career: {state}")
    return state


def contract_data(user: Path, stage: str) -> bytes:
    path = user / smoke_run.SAVE_NAME
    if not path.is_file() or path.stat().st_size < 1024:
        raise AssertionError(f"{stage}: native .sav missing or empty")
    data = Sav(str(path)).read_member("contract.dat")
    if not data:
        raise AssertionError(f"{stage}: native contract.dat is empty")
    return data


def main(args):
    if len(args) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, args[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        raise AssertionError("pass existing native executable and game directory")

    with tempfile.TemporaryDirectory(prefix="owner-game-native-offer-") as tmp:
        root = Path(tmp)
        before_dir = root / "before" / "user"
        after_dir = root / "after" / "user"

        # Start a NEW scripted Arsenal career; the native assistant handles
        # first-team contracts while the human manager is on holiday.
        create_steps = [s for s in smoke_run.STEPS if " dump " not in s]
        create_log = run_native(
            program, game, before_dir, create_steps,
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "5"},
            "native Arsenal offer and save"
        )
        adds = set(ADD.findall(create_log))
        days = set(DAILY.findall(create_log))
        observed = adds & days
        if not observed:
            raise AssertionError(
                f"native game did not produce a human-club offer which "
                f"survived through at least one daily update; "
                f"added={sorted(adds)[:8]}, daily={sorted(days)[:8]}\n"
                + create_log[-2400:]
            )
        original = owner_state(before_dir, "pre-reload")
        contract_before = contract_data(before_dir, "pre-reload")
        print(f"PASS native offer events for {len(observed)} staff/club pairs: "
              f"{sorted(observed)[:6]}", flush=True)
        print(f"PASS native save with contract.dat "
              f"{len(contract_before)} decoded bytes", flush=True)

        # A fresh process and user directory. The source save was made while
        # the manager was on holiday, so DO NOT wait for a first-news screen:
        # the holiday can legitimately continue without an interactive turn.
        after_dir.mkdir(parents=True)
        shutil.copy2(before_dir / smoke_run.SAVE_NAME,
                     after_dir / smoke_run.SAVE_NAME)
        reload_steps = fixtures.LOAD_STEPS[:-1] + ["180000 quit"]
        run_native(
            program, game, after_dir, reload_steps,
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "0"},
            "reload and re-save native contract state"
        )
        restored = owner_state(after_dir, "post-reload")
        contract_after = contract_data(after_dir, "post-reload")
        if restored["club_id"] != original["club_id"]:
            raise AssertionError("Arsenal identity changed over save/reload")
        if restored["date"] != original["date"]:
            raise AssertionError(
                f"Game date changed during roundtrip: "
                f"{original['date']} -> {restored['date']}; "
                "the test must capture the same native game turn"
            )
        if contract_before != contract_after:
            raise AssertionError(
                "native contract.dat changed after loading and saving "
                "the same game turn: before sha256="
                f"{hashlib.sha256(contract_before).hexdigest()}, after="
                f"{hashlib.sha256(contract_after).hexdigest()}"
            )
        print(f"PASS native contract save/reload: exact "
              f"{len(contract_before)}-byte contract.dat roundtrip, "
              "same Arsenal and same game date", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError, KeyError) as exc:
        print(f"FAIL native offer save/reload: {exc}", file=sys.stderr)
        sys.exit(1)
