#!/usr/bin/env python3
"""Map the native Arsenal player Offer Contract UI for a deterministic test.

Runs only in an isolated new Arsenal career, navigates to the human club's
squad, opens a player's profile and their Action menu, and saves screen dumps
and native screen-log evidence. No contract values or player data are changed.

This is a diagnostic mapping fixture, not a contract-persistence pass gate.
"""
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "fixtures"))
sys.path.insert(0, str(ROOT / "smoke"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402


def main(argv):
    if len(argv) != 4:
        print("Usage: offer_ui_map.py <native executable> <game directory> <output directory>")
        return 2
    program, game, out = map(os.path.abspath, argv[1:])
    if not Path(program).is_file() or not Path(game).is_dir():
        raise AssertionError("Missing executable or game data")
    Path(out).mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="og-offer-ui-map-") as tmp:
        user = Path(tmp) / "user"
        user.mkdir()
        fixtures.write_settings(str(user), "classic", [("changing_rules", "off")])
        steps = [s for s in smoke_run.STEPS if " dump " not in s]
        steps = steps[:steps.index("wait first-news") + 1]
        steps += [
            "1200 click 45 165",            # Manager menu
            "3000 click 150 156",           # Arsenal Squad
            "wait screen:0x454900",
            "1200 dump {d}/01-arsenal-squad.bmp",
            "2000 click 210 234",           # First-team player
            "wait screen:0x870700",
            "1200 dump {d}/02-player-profile.bmp",
            "1800 click 722 14",            # Action menu
            "wait popup",
            "1200 dump {d}/03-player-action-menu.bmp",
            "1800 click 690 80",            # Native Offer New Contract action
            "3000 dump {d}/04-offer-contract-screen.bmp",
            "5000 quit",
        ]
        result, duration = fixtures.run(
            program, game, "classic", str(user),
            smoke_run.scaled_steps("classic", out, steps),
            {"RECOMP_SCREEN_LOG": "1", "RECOMP_OWNER_CONTRACT_TRACE": "1"},
            timeout=900,
        )
        (Path(out) / "native-screen-log.txt").write_text(
            result.stderr, encoding="utf-8", errors="replace"
        )
        if result.returncode:
            raise AssertionError(f"UI mapping failed (exit {result.returncode}); "
                                 f"{result.stderr[-1500:]}")
        shots = sorted(Path(out).glob("*.bmp"))
        if len(shots) != 4:
            raise AssertionError(
                f"Expected squad, profile, Action-menu and contract-screen dumps; "
                f"got {[p.name for p in shots]}. See native screen log."
            )
        screen_log = [
            line for line in result.stderr.splitlines()
            if "recomp: screen " in line or "recomp: script " in line
        ]
        print(f"PASS mapped 4 native player UI screens after {duration:.1f}s",
              flush=True)
        print("\n".join(screen_log[-25:]), flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError) as error:
        print(f"FAIL owner-game offer UI mapping: {error}", file=sys.stderr)
        sys.exit(1)
