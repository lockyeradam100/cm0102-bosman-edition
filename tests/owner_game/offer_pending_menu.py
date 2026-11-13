#!/usr/bin/env python3
"""Inspect a real pending Seaman offer after saving and reloading CM01/02.

Uses offer_roundtrip's already-proven native UI submission route. The second,
new game process loads that isolated .sav, opens Seaman's Action menu, then
captures it for inspection. The menu must subsequently be used to verify the
native pending-offer lifecycle; merely dumping a menu is NOT a persistence pass.
"""
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from offer_roundtrip import (  # noqa: E402
    ARSENAL_CLUB_ID, SEAMAN_STAFF_ID, NATIVE_ADD, offer_steps, run_native,
    native_contracts,
)
import fixtures  # noqa: E402
import smoke_run  # noqa: E402


def main(args):
    if len(args) != 4:
        print("Usage: offer_pending_menu.py <game binary> <game folder> <screen output>")
        return 2
    program, game, out = map(os.path.abspath, args[1:])
    Path(out).mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="owner-game-pending-menu-") as tmp:
        first = Path(tmp) / "offer" / "user"
        second = Path(tmp) / "reload" / "user"
        log = run_native(
            program, game, first, offer_steps(),
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "1"},
            "submit native Seaman contract and save next day",
        )
        if (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) not in set(NATIVE_ADD.findall(log)):
            raise AssertionError("No confirmed Seaman native contract offer")
        before = native_contracts(first, "submitted Seaman offer")
        second.mkdir(parents=True)
        shutil.copy2(first / smoke_run.SAVE_NAME,
                     second / smoke_run.SAVE_NAME)
        fixtures.write_settings(str(second), "classic", [
            ("changing_rules", "off"), ("faster_days", "on")
        ])
        steps = fixtures.LOAD_STEPS + [
            "1200 click 45 165",            # Manager menu
            "3000 click 150 156",           # Arsenal squad
            "wait screen:0x454900",
            "2000 click 210 234",           # Same Seaman native player
            "wait screen:0x870700",
            "1800 click 722 14",            # Action
            "wait popup",
            "1200 dump {d}/01-seaman-action-after-reload.bmp",
            "5500 quit",
        ]
        result, took = fixtures.run(
            program, game, "classic", str(second),
            smoke_run.scaled_steps("classic", out, steps),
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SCREEN_LOG": "1"},
            timeout=900,
        )
        (Path(out) / "screen-log.txt").write_text(
            result.stderr, encoding="utf-8", errors="replace",
        )
        if result.returncode:
            raise AssertionError(f"CM reload/menu exit {result.returncode} "
                                 f"after {took:.1f}s\n{result.stderr[-2300:]}")
        shot = Path(out) / "01-seaman-action-after-reload.bmp"
        if not shot.is_file():
            raise AssertionError("No screenshot of reloaded Seaman Action menu")
        print("PASS reloaded native Arsenal career and reopened Seaman Action "
              "menu in a fresh game process; screenshot available for "
              "pending-offer inspection", flush=True)
        print("NOTE: visual inspection and a subsequent native withdraw/"
              "pending-state assertion are required before certifying "
              "the pending offer survived", flush=True)
        print(f"Saved decoded contract.dat length before reload: {len(before)}",
              flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError, KeyError) as error:
        print(f"FAIL Seaman post-reload pending menu probe: {error}", file=sys.stderr)
        sys.exit(1)
