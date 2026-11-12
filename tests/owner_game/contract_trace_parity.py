#!/usr/bin/env python3
"""Run identical native CM01/02 careers with contract tracing OFF and ON.

The diagnostic must be observational: same database, fixed random seed,
identical clicks and game days must produce the same binary native save.
First compare two trace-disabled controls to detect baseline nondeterminism;
report changed native save parts without ignoring any differences.
Also verifies that the ContractManager native DAILY hook really executed,
rather than passing because trace code was never reached.

Usage: python3 tests/owner_game/contract_trace_parity.py <native app binary> <game folder>

Uses completely temporary RECOMP_USER_DIR locations. Does not touch player
saves. This is a compatibility regression, not a submitted-offer acceptance
or full contract persistence test.
"""
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


def run_case(program: str, game: str, root: Path, label: str, enable_trace: bool):
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
    return label, str(saved), hashlib.sha256(saved.read_bytes()).hexdigest(), saved.stat().st_size, result.stderr


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
        # Separate native control runs expose fixture nondeterminism. Serial
        # execution removes any CPU scheduling interaction between processes.
        off_a = run_case(program, game, root, "trace-off-a", False)
        off_b = run_case(program, game, root, "trace-off-b", False)
        on = run_case(program, game, root, "trace-on", True)
        for label, _, digest, size, _ in (off_a, off_b, on):
            print(f"PASS {label}: real CM save {size} bytes, sha256 {digest}", flush=True)

        def explain_diff(a, b):
            a_parts = smoke_run.save_parts(a[1])
            b_parts = smoke_run.save_parts(b[1])
            differences = sorted(k for k in set(a_parts) | set(b_parts)
                                 if a_parts.get(k) != b_parts.get(k))
            print(f"PART DIFF {a[0]} vs {b[0]}: "
                  + (", ".join(differences) or "file header only"), flush=True)
            for name in differences[:20]:
                left, right = a_parts.get(name), b_parts.get(name)
                print(f"  {name}: "
                      f"stored bytes {len(left[0]) if left else 'MISSING'} -> "
                      f"{len(right[0]) if right else 'MISSING'}, "
                      f"unpacked {left[1] if left else 'MISSING'} -> "
                      f"{right[1] if right else 'MISSING'}", flush=True)
            return differences

        if off_a[2] != off_b[2]:
            explain_diff(off_a, off_b)
            raise AssertionError(
                "BASELINE NON-DETERMINISM: two runs with tracing disabled "
                "already produce different native saves. Stabilise the "
                "fixture before attributing changes to diagnostics."
            )
        print("PASS control repeatability: two trace-disabled native saves identical",
              flush=True)

        if off_a[2] != on[2]:
            explain_diff(off_a, on)
            trace_lines = [line for line in on[4].splitlines()
                           if "owner-game: contract" in line]
            print(f"TRACE EVENTS ({len(trace_lines)} total): "
                  + "; ".join(trace_lines[-12:]), flush=True)
            raise AssertionError(
                "CONTRACT TRACE CHANGED native saved state (controls identical). "
                "Do not ship until the changed save parts are explained."
            )
        print("PASS native contract trace is observational: identical complete saves",
              flush=True)

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError) as error:
        print(f"FAIL Owner Game contract trace parity: {error}", file=sys.stderr)
        sys.exit(1)
