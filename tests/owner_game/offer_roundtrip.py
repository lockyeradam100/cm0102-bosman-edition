#!/usr/bin/env python3
"""Owner Game P1: REAL human contract offer -> native .sav -> reload -> re-save.

A fresh Arsenal manager deliberately offers goalkeeper David Seaman the
ordinary terms shown by Championship Manager's native contract interface.
The scripted clicks select his role, click Offer, then confirm YES. The game
must call ContractManager::add_contract_offer(staff 6528, Arsenal club 362),
advance one actual game day, and save its real contract.dat.

A separate same-date Arsenal control career makes NO offer: its native
contract.dat must differ from the offered career, proving that a real contract
state difference was saved rather than only logged in the UI.

Then a distinct native process reloads exactly the offered .sav and re-saves
at its first human turn with no new action or owner command. The date, club,
and decoded contract.dat MUST match the offered save exactly.

This does not claim that a reply was received or that the offer was accepted:
those are subsequent native resolver / response tests. The test never
touches an existing player career and does not create fictitious offers.

Usage: offer_roundtrip.py <native executable> <game directory>
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

NATIVE_ADD = re.compile(
    r"owner-game: contract native_add_called staff=(\d+) club=(\d+)"
)
SEAMAN_STAFF_ID = "6528"
ARSENAL_CLUB_ID = "362"


def run_native(program: str, game: str, user: Path,
               steps: list[str], env: dict[str, str], label: str) -> str:
    user.mkdir(parents=True, exist_ok=True)
    fixtures.write_settings(str(user), "classic", [
        ("faster_days", "on"),
        ("changing_rules", "off"),
    ])
    result, elapsed = fixtures.run(
        program, game, "classic", str(user),
        smoke_run.scaled_steps("classic", str(user), steps),
        env, timeout=900,
    )
    if result.returncode:
        raise AssertionError(f"{label}: native executable exit "
                             f"{result.returncode} after {elapsed:.1f}s\n"
                             + result.stderr[-3500:])
    if "recomp: saved" not in result.stderr:
        raise AssertionError(f"{label}: native .sav was not written\n"
                             + result.stderr[-3500:])
    print(f"PASS {label}: native CM save written after {elapsed:.1f}s", flush=True)
    return result.stderr


def owner_state(user: Path, label: str) -> dict:
    p = user / "owner-game-state.json"
    if not p.is_file():
        raise AssertionError(f"{label}: missing native Owner Game bridge state")
    data = json.loads(p.read_text(encoding="utf-8"))
    if data.get("ready") is not True or data.get("club_name") != "Arsenal":
        raise AssertionError(f"{label}: wrong career or bridge not ready: {data}")
    if str(data.get("club_id")) != ARSENAL_CLUB_ID:
        raise AssertionError(f"{label}: not the real Arsenal club: {data}")
    return data


def native_contracts(user: Path, label: str) -> bytes:
    saved = user / smoke_run.SAVE_NAME
    if not saved.is_file() or saved.stat().st_size < 1024:
        raise AssertionError(f"{label}: no native career saved")
    data = Sav(str(saved)).read_member("contract.dat")
    if not data:
        raise AssertionError(f"{label}: missing native contract manager data")
    return data


def offer_steps() -> list[str]:
    steps = [s for s in smoke_run.STEPS if " dump " not in s]
    steps = steps[:steps.index("wait first-news") + 1]
    return steps + [
        "1200 click 45 165",          # Manager menu
        "3000 click 150 156",         # Arsenal squad
        "wait screen:0x454900",
        "2000 click 210 234",         # David Seaman's native profile
        "wait screen:0x870700",
        "1800 click 722 14",          # Action dropdown
        "wait popup",
        "1800 click 690 80",          # Offer New Contract
        "4800 click 272 216",         # Important first-team player
        "7500 click 705 573",         # Set Role At Club
        "wait screen:0x4deac0",
        "3500 click 705 573",         # Offer Contract (CM's demands)
        "6500 click 705 573",         # Yes, genuinely submit offer
        "8500 click 45 115",          # Continue Game: advance one real day
        "180000 quit",                # Fails safely if native autosave not made
    ]


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, argv[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        raise AssertionError("Missing native game binary or packaged game directory")

    with tempfile.TemporaryDirectory(prefix="owner-game-real-offer-roundtrip-") as tmp:
        root = Path(tmp)
        first = root / "new" / "user"
        second = root / "reloaded" / "user"
        control = root / "control-no-offer" / "user"

        # One-day delayed native autosave, after the real offer was submitted.
        first_log = run_native(
            program, game, first, offer_steps(),
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "1"},
            "submit Seaman offer, advance one game day and save",
        )
        offered = set(NATIVE_ADD.findall(first_log))
        if (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) not in offered:
            raise AssertionError(
                f"CM did not register Seaman's Arsenal offer; got {sorted(offered)}"
            )
        print("PASS CM native ContractManager offer: David Seaman "
              "(staff 6528) to Arsenal (club 362)", flush=True)
        original = owner_state(first, "new career")
        contract_before = native_contracts(first, "after submitting offer")
        print(f"PASS saved contract.dat: {len(contract_before)} decoded bytes, "
              f"SHA256 {hashlib.sha256(contract_before).hexdigest()}", flush=True)

        # Crucial independent proof that the *offer itself* changed the
        # native saved contract state. Identical save/reload hashes alone
        # could otherwise mask an offer that was never persisted.
        base_steps = [x for x in smoke_run.STEPS if " dump " not in x]
        base_steps = base_steps[:base_steps.index("wait first-news") + 1]
        base_steps += [
            "1200 space",                   # Real Continue Game shortcut (default on)
            "180000 quit",
        ]
        run_native(
            program, game, control, base_steps,
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "1",
             "RECOMP_SCREEN_LOG": "1"},
            "control Arsenal career with no contract submission",
        )
        control_state = owner_state(control, "no-offer control")
        if control_state["date"] != original["date"]:
            raise AssertionError(
                "Control and offered careers do not share the same game date: "
                f"{control_state['date']} vs {original['date']}"
            )
        baseline = native_contracts(control, "no-offer control")
        if baseline == contract_before:
            raise AssertionError(
                "Real Seaman submit event was observed, but offered career's "
                "contract.dat exactly matches no-offer control. Pending offer "
                "was not demonstrably saved; do not claim persistence."
            )
        changes = sum(a != b for a, b in zip(baseline, contract_before))
        changes += abs(len(baseline) - len(contract_before))
        print("PASS offer changed NATIVE SAVED CONTRACT STATE relative "
              f"to a same-date no-offer control: {changes} decoded bytes differ, "
              f"control len={len(baseline)}, offered len={len(contract_before)}",
              flush=True)


        # Reload in a separate process, isolated HOME/RECOMP_USER_DIR, no owner
        # sidecars or commands; save immediately at its first native human turn.
        second.mkdir(parents=True)
        shutil.copy2(first / smoke_run.SAVE_NAME,
                     second / smoke_run.SAVE_NAME)
        # Unlike the abandoned holiday fixture, the manager is NOT on holiday;
        # a valid reload can reach the native first-news screen.
        load_steps = fixtures.LOAD_STEPS + ["180000 quit"]
        second_log = run_native(
            program, game, second, load_steps,
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "0"},
            "reload native career and re-save without a transaction",
        )
        restored = owner_state(second, "reloaded career")
        contract_after = native_contracts(second, "reloaded career")
        if restored["club_id"] != original["club_id"]:
            raise AssertionError("Native Arsenal club ID changed on reload")
        if restored["date"] != original["date"]:
            raise AssertionError(
                f"Native CM date changed on reload: {original['date']} -> "
                f"{restored['date']}; cannot compare a different game turn"
            )
        if contract_before != contract_after:
            raise AssertionError(
                "Native contract.dat changed across immediate load/save: "
                f"before {hashlib.sha256(contract_before).hexdigest()}, "
                f"after {hashlib.sha256(contract_after).hexdigest()}"
            )
        if (second / "owner-game-command.txt").exists():
            raise AssertionError("Stale owner command appeared in fresh process")
        print("PASS REAL CM contract save/reload: submitted Seaman offer, "
              f"same Arsenal, same game date, identical {len(contract_after)}-byte "
              "decoded contract.dat", flush=True)
        # An accepted/rejected decision must be validated with the native
        # resolver in a later test. Do not infer it from lack of a reply.
        if "cm_accepted_human_club_offer" in second_log:
            print("NOTICE: native decision event observed during reload", flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError, KeyError) as error:
        print(f"FAIL real native offer save/reload: {error}", file=sys.stderr)
        sys.exit(1)
