#!/usr/bin/env python3
"""Manager Desk native-market observational parity gate.

Runs three independently generated Arsenal test careers. Two have the native
manager observer disabled; the third enables it. All use the same seed, fixed
clock and five in-game days, with CM's ordinary holiday simulation.

This gate must prove that read-only manager hooks are actually exercised and
do not change any native .sav bytes. A daily hook marker is required, but
the test NEVER claims an appointment or dismissal if none happens naturally.

Usage: manager_trace_parity.py <native Mac executable> <game directory>
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

MARKER = "owner-game: manager daily_market_observed"
TRACE = "owner-game-manager-native.log"


def run_case(program: str, game: str, root: Path,
             name: str, enabled: bool) -> tuple[bytes, str]:
    user = root / name / "user"
    user.mkdir(parents=True)
    fixtures.write_settings(str(user), "classic", [
        ("faster_days", "on"),
        ("changing_rules", "off"),
    ])
    steps = [s for s in smoke_run.STEPS if " dump " not in s]
    result, elapsed = fixtures.run(
        program, game, "classic", str(user),
        smoke_run.scaled_steps("classic", str(user), steps),
        {
            "RECOMP_SAVE_AFTER_DAYS": "5",
            "RECOMP_OWNER_MANAGER_TRACE": "1" if enabled else "0",
            "RECOMP_OWNER_CONTRACT_TRACE": "0",
        }, timeout=900,
    )
    if result.returncode:
        raise AssertionError(
            f"{name}: native game exited {result.returncode} "
            f"after {elapsed:.1f}s\n{result.stderr[-2700:]}"
        )
    if "recomp: saved" not in result.stderr:
        raise AssertionError(f"{name}: no native autosave after five days")
    path = user / smoke_run.SAVE_NAME
    if not path.is_file() or path.stat().st_size < 1024:
        raise AssertionError(f"{name}: missing native .sav")
    if (MARKER in result.stderr) != enabled:
        raise AssertionError(f"{name}: unexpected native manager observer state")
    log = user / TRACE
    if enabled:
        if not log.is_file() or "event=daily_market_observed" not in \
                log.read_text(encoding="utf-8"):
            raise AssertionError(
                "The native manager daily hook did not generate its event"
            )
        print(f"PASS {name}: native manager daily engine observed, "
              "without replacing its outcome", flush=True)
    elif log.exists():
        raise AssertionError(
            f"{name}: manager trace file exists despite observer being OFF"
        )
    data = path.read_bytes()
    print(
        f"PASS {name}: native save {len(data)} bytes; "
        f"SHA256 {hashlib.sha256(data).hexdigest()}", flush=True,
    )
    return data, result.stderr


def main(args: list[str]) -> int:
    if len(args) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, args[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        raise AssertionError("Missing native binary or packaged game directory")

    with tempfile.TemporaryDirectory(prefix="owner-manager-native-parity-") as tmp:
        root = Path(tmp)
        off_a, _ = run_case(program, game, root, "no-observer-a", False)
        off_b, _ = run_case(program, game, root, "no-observer-b", False)
        on, _ = run_case(program, game, root, "manager-observer-on", True)

        if off_a != off_b:
            raise AssertionError(
                "The untouched native baseline is not deterministic; "
                "cannot attribute any differences to Manager Desk"
            )
        print("PASS two native trace-OFF control saves are exactly identical",
              flush=True)
        if off_a != on:
            raise AssertionError(
                "Manager Desk observer altered native football save bytes; "
                "disable hook until native parity is restored"
            )
        print("PASS manager observer does not change ANY native save bytes",
              flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError) as err:
        print(f"FAIL Manager Desk native parity: {err}", file=sys.stderr)
        sys.exit(1)
