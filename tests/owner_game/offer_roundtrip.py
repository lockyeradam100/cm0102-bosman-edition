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

The native readback additionally confirms an open offer for Seaman at the
same save turn, absent in the cancelled control, and still open after
reloading into a distinct process. This is an explicit opt-in diagnostic
from the native ContractManager at the normal human-turn safe point.

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
SAFE_POINT = re.compile(
    r"owner-game: contract safe_point staff=(\d+) club=(\d+) "
    r"status=(open_offer_present|open_offer_absent|unavailable)"
)


def safe_point_status(log: str, label: str) -> str:
    observed = [
        status for staff, club, status in SAFE_POINT.findall(log)
        if staff == SEAMAN_STAFF_ID and club == ARSENAL_CLUB_ID
    ]
    if not observed:
        raise AssertionError(
            f"{label}: missing independent native open-offer readback "
            "at a human turn"
        )
    if observed[-1] == "unavailable":
        raise AssertionError(
            f"{label}: ContractManager not available for safe-point readback"
        )
    print(f"CHECK {label}: native Seaman offer {observed[-1]} "
          f"({len(observed)} observed human turns)", flush=True)
    return observed[-1]



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
        no_watch = root / "same-offer-no-readback" / "user"

        # One-day delayed native autosave, after the real offer was submitted.
        first_log = run_native(
            program, game, first, offer_steps(),
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "1",
             "RECOMP_OWNER_CONTRACT_WATCH_STAFF": SEAMAN_STAFF_ID},
            "submit Seaman offer, advance one game day and save",
        )
        offered = set(NATIVE_ADD.findall(first_log))
        if (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) not in offered:
            raise AssertionError(
                f"CM did not register Seaman's Arsenal offer; got {sorted(offered)}"
            )
        print("PASS CM native ContractManager offer: David Seaman "
              "(staff 6528) to Arsenal (club 362)", flush=True)
        first_pending = safe_point_status(first_log, "after offer day")
        if first_pending != "open_offer_present":
            raise AssertionError(
                "The native offer submit event happened, but the offer "
                "is not in the native open-offer list on the saved turn. "
                "This does NOT establish player rejection."
            )
        original = owner_state(first, "new career")
        contract_before = native_contracts(first, "after submitting offer")
        print(f"PASS saved contract.dat: {len(contract_before)} decoded bytes, "
              f"SHA256 {hashlib.sha256(contract_before).hexdigest()}", flush=True)

        # Crucial independent proof that the *offer itself* changed the
        # native saved contract state. Identical save/reload hashes alone
        # could otherwise mask an offer that was never persisted.
        # Do EXACTLY the same player-profile and contract-screen actions,
        # but say No to the final confirmation. This avoids a confounded
        # control that gets stuck at first-news without visiting Arsenal.
        # The additional Cancel/Back clicks close the contract forms.
        base_steps = offer_steps()
        yes = base_steps.index("6500 click 705 573")
        base_steps[yes] = "6500 click 350 573"   # No, do not submit
        base_steps.insert(yes + 1, "7300 click 350 573")  # Cancel the terms form
        base_steps.insert(yes + 2, "7900 click 350 573")  # Back if role screen remains
        control_log = run_native(
            program, game, control, base_steps,
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "1",
             "RECOMP_SCREEN_LOG": "1",
             "RECOMP_OWNER_CONTRACT_WATCH_STAFF": SEAMAN_STAFF_ID},
            "control Arsenal career choosing No to Seaman offer",
        )
        control_status = safe_point_status(control_log, "cancel control")
        if control_status != "open_offer_absent":
            raise AssertionError(
                "Cancelled-offer control still shows a live Seaman offer"
            )
        control_offers = set(NATIVE_ADD.findall(control_log))
        if (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) in control_offers:
            raise AssertionError("No-offer control unexpectedly submitted Seaman's offer")
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

        # The optional readback must not affect gameplay or saved contract
        # state. Submit the IDENTICAL native Seaman offer again, but leave
        # RECOMP_OWNER_CONTRACT_WATCH_STAFF completely unset.
        other_log = run_native(
            program, game, no_watch, offer_steps(),
            {"RECOMP_OWNER_CONTRACT_TRACE": "1",
             "RECOMP_SAVE_AFTER_DAYS": "1"},
            "same Seaman offer with safe-point readback disabled",
        )
        if (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) not in set(NATIVE_ADD.findall(other_log)):
            raise AssertionError("Unwatched control did not submit native offer")
        if owner_state(no_watch, "readback-off")["date"] != original["date"]:
            raise AssertionError("Readback-off control game date changed")
        other_contract = native_contracts(no_watch, "readback-off")
        if other_contract != contract_before:
            raise AssertionError(
                "Opt-in native safe-point readback CHANGED the saved contract "
                "state; do not ship diagnostics. watched_sha256="
                f"{hashlib.sha256(contract_before).hexdigest()} "
                f"unwatched_sha256={hashlib.sha256(other_contract).hexdigest()}"
            )
        print("PASS observer parity: identical native offer saved exactly the "
              "same contract.dat with safe-point readback ON and OFF", flush=True)



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
             "RECOMP_SAVE_AFTER_DAYS": "0",
             "RECOMP_OWNER_CONTRACT_WATCH_STAFF": SEAMAN_STAFF_ID},
            "reload native career and re-save without a transaction",
        )
        reloaded_status = safe_point_status(second_log, "after reload")
        if reloaded_status != first_pending:
            raise AssertionError(
                f"Native offer status changed during save/reload: "
                f"{first_pending} -> {reloaded_status}"
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
