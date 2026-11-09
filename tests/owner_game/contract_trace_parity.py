#!/usr/bin/env python3
"""Run identical native CM01/02 careers with contract tracing OFF and ON.

The diagnostic must be observational: same database, fixed random seed,
identical clicks and game days must produce the same binary native save.
Also verifies that the ContractManager native DAILY hook really executed,
rather than passing because trace code was never reached.

Usage: python3 tests/owner_game/contract_trace_parity.py <native app binary> <game folder>

Uses completely temporary RECOMP_USER_DIR locations. Does not touch player
saves. This is a compatibility regression, not a submitted-offer acceptance
or full contract persistence test.
"""
import concurrent.futures
import hashlib
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "fixtures"))
sys.path.insert(0, str(ROOT / "smoke"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402


def run_case(program: str, game: str, root: Path, enable_trace: bool):
    label = "trace-on" if enable_trace else "trace-off"
    user = root / label / "user"
    user.mkdir(parents=True)
    fixtures.write_settings(str(user), "classic", [("faster_days", "on")])

    # The original Bosman smoke fixture: create Arsenal 2001 and use native
    # Go on Holiday to let CM simulate five days of real club activity.
    steps = [s for s in smoke_run.STEPS if " dump " not in s]
    env = {
        "RECOMP_SAVE_AFTER_DAYS": "5",
        "RECOMP_OWNER_CONTRACT_TRACE": "1" if enable_trace else "0",
    }
    result, took = fixtures.run(
        program, game, "classic", str(user),
        smoke_run.scaled_steps("classic", str(user), steps), env, timeout=900,
    )
    if result.returncode:
        raise AssertionError(f"{label}: native game exited {result.returncode} "
                             f"after {took:.1f}s\n{result.stderr[-2500:]}")
    if "recomp: saved" not in result.stderr:
        raise AssertionError(f"{label}: no native save confirmation\n"
                             f"{result.stderr[-2500:]}")
    saved = user / smoke_run.SAVE_NAME
    if not saved.is_file() or saved.stat().st_size < 1024:
        raise AssertionError(f"{label}: no valid native saved career")

    marker = "owner-game: contract lifecycle trace active at native daily boundary"
    if (marker in result.stderr) != enable_trace:
        raise AssertionError(f"{label}: unexpected contract tracing state "
                             f"(expected active={enable_trace})")
    return label, hashlib.sha256(saved.read_bytes()).hexdigest(), saved.stat().st_size


def main(args):
    if len(args) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, args[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        print("ERROR: need existing program and game paths")
        return 2

    with tempfile.TemporaryDirectory(prefix="owner-contract-parity-") as folder:
        root = Path(folder)
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            futures = [
                pool.submit(run_case, program, game, root, trace)
                for trace in (False, True)
            ]
            results = [f.result() for f in futures]

        for label, digest, size in results:
            print(f"PASS {label}: real CM save {size} bytes, sha256 {digest}")
        if results[0][1] != results[1][1]:
            raise AssertionError("Native football saved state CHANGED with "
                                 "read-only contract lifecycle tracing enabled")
        print("PASS native contract trace is observational: identical full saves")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError) as error:
        print(f"FAIL Owner Game contract trace parity: {error}", file=sys.stderr)
        sys.exit(1)
