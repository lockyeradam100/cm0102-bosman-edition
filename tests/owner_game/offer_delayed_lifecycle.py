#!/usr/bin/env python3
"""Native, isolated delayed-offer lifecycle regression for Owner Game.

Seaman's contract offer is submitted through CM's actual Arsenal UI, saved,
and reloaded. The reloaded Arsenal manager delegates a week to CM's native
holiday simulation; the game's own daily contract machinery decides whether
the offer is still pending, removed, or has led to a genuine signing.

A second, identical career runs with the optional offer readback disabled.
Their decoded native contract.dat MUST match. A third process reloads the
advanced career and proves that its final pending state and contract data
survive another save/reload. No existing user career is accessed.

An offer disappearing is not by itself a rejection. Only the native
sign_contract success callback can establish an actual signing.
"""
import hashlib
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from offer_roundtrip import (  # noqa: E402
    ARSENAL_CLUB_ID,
    SEAMAN_STAFF_ID,
    NATIVE_ADD,
    SAFE_POINT,
    native_contracts,
    offer_steps,
    owner_state,
    run_native,
    safe_point_status,
)
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

SIGNED = re.compile(
    r"owner-game: contract native_sign_succeeded staff=(\d+) club=(\d+)"
)


def copy_native_save(source: Path, target: Path) -> None:
    target.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / smoke_run.SAVE_NAME, target / smoke_run.SAVE_NAME)


def holiday_steps() -> list[str]:
    """Reload an actual save, then use the same native holiday as CM smoke."""
    return fixtures.LOAD_STEPS + [
        "1200 click 45 165",       # Human manager's menu
        "4500 click 150 427",      # Native Go on Holiday
        "7000 click 705 572",      # Confirm Yes
        "300000 quit",             # The native day-count autosave ends earlier
    ]


def history(log: str) -> list[str]:
    return [
        status for staff, club, status in SAFE_POINT.findall(log)
        if staff == SEAMAN_STAFF_ID and club == ARSENAL_CLUB_ID
    ]


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, argv[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        raise AssertionError("Missing packaged Mac executable or game directory")

    with tempfile.TemporaryDirectory(prefix="owner-native-delayed-offer-") as temp:
        root = Path(temp)
        submitted = root / "submitted" / "user"
        observed = root / "one-week-watch" / "user"
        control = root / "one-week-control" / "user"
        reloaded = root / "reloaded-week" / "user"

        original_log = run_native(
            program, game, submitted, offer_steps(),
            {
                "RECOMP_OWNER_CONTRACT_TRACE": "1",
                "RECOMP_SAVE_AFTER_DAYS": "1",
                "RECOMP_OWNER_CONTRACT_WATCH_STAFF": SEAMAN_STAFF_ID,
            },
            "submit a real native Seaman offer",
        )
        if (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) not in set(
            NATIVE_ADD.findall(original_log)
        ):
            raise AssertionError("Native contract offer was not submitted")
        if safe_point_status(original_log, "submitted Seaman offer") != \
                "open_offer_present":
            raise AssertionError("Submitted native offer was not open")
        original_state = owner_state(submitted, "submission career")

        # One fork must not observe or influence another: all game files,
        # HOME directories, careers and owner bridge files are isolated.
        copy_native_save(submitted, observed)
        copy_native_save(submitted, control)
        env = {
            "RECOMP_OWNER_CONTRACT_TRACE": "1",
            "RECOMP_SAVE_AFTER_DAYS": "7",
        }
        observed_log = run_native(
            program, game, observed, holiday_steps(),
            {**env, "RECOMP_OWNER_CONTRACT_WATCH_STAFF": SEAMAN_STAFF_ID},
            "CM advances seven days with native offer readback",
        )
        control_log = run_native(
            program, game, control, holiday_steps(), env,
            "CM advances identical seven days without offer readback",
        )
        stages = history(observed_log)
        if not stages or stages[0] != "open_offer_present":
            raise AssertionError(
                "Fresh-process native reload did not see the pending offer"
            )
        if any(status == "unavailable" for status in stages):
            raise AssertionError("ContractManager unavailable during readback")
        final = stages[-1]
        advanced_state = owner_state(observed, "advanced career")
        control_state = owner_state(control, "non-observed career")
        if advanced_state["date"] <= original_state["date"]:
            raise AssertionError("The native game did not advance")
        if advanced_state["date"] != control_state["date"]:
            raise AssertionError("Readback changed the final game date")

        observed_contract = native_contracts(observed, "watched week")
        control_contract = native_contracts(control, "unwatched week")
        if observed_contract != control_contract:
            raise AssertionError(
                "Optional readback changed the native saved contracts: "
                f"watched {hashlib.sha256(observed_contract).hexdigest()}, "
                f"unwatched {hashlib.sha256(control_contract).hexdigest()}"
            )
        if history(control_log):
            raise AssertionError("Unwatched control unexpectedly performed readback")
        print(
            "PASS seven-day observer parity: identical native contract.dat "
            "with readback enabled and disabled", flush=True,
        )

        signed = (SEAMAN_STAFF_ID, ARSENAL_CLUB_ID) in set(
            SIGNED.findall(observed_log)
        )
        if signed:
            print("PASS actual native sign_contract succeeded for Seaman "
                  "and Arsenal after the submitted offer", flush=True)
        elif final == "open_offer_present":
            print("OBSERVED native Seaman offer is still pending: "
                  "no signing and no invented rejection", flush=True)
        else:
            print("OBSERVED native Seaman offer is no longer on the pending "
                  "list, but no signing was observed; reason remains "
                  "UNCLASSIFIED, not a claimed rejection", flush=True)

        copy_native_save(observed, reloaded)
        reread_log = run_native(
            program, game, reloaded,
            fixtures.LOAD_STEPS + ["180000 quit"],
            {
                "RECOMP_OWNER_CONTRACT_TRACE": "1",
                "RECOMP_SAVE_AFTER_DAYS": "0",
                "RECOMP_OWNER_CONTRACT_WATCH_STAFF": SEAMAN_STAFF_ID,
            },
            "reload the advanced offer career in a third process",
        )
        if safe_point_status(reread_log, "second reload") != final:
            raise AssertionError(
                "Native pending status differs after second reload: "
                f"{final} -> {history(reread_log)[-1]}"
            )
        if owner_state(reloaded, "second reload")["date"] != \
                advanced_state["date"]:
            raise AssertionError("The second reload changed the game date")
        if native_contracts(reloaded, "second reload") != observed_contract:
            raise AssertionError("The second reload altered saved contract.dat")
        print(
            f"PASS delayed native lifecycle: {len(stages)} safe-point observations "
            f"through {advanced_state['date']}; final={final}; "
            "third-process contract save/reload exact", flush=True,
        )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, TimeoutError, KeyError) as err:
        print(f"FAIL delayed native offer lifecycle: {err}", file=sys.stderr)
        sys.exit(1)
