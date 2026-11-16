#!/usr/bin/env python3
"""Native Manager Desk proof: delegate, then cancel before the next fixture.

Runs a fresh, isolated Arsenal 2001 test career with CM's real executable and
its scripted UI. Injects authenticated owner commands *from outside the game*
at the active human-turn boundary, exactly as the Owner Game SDL menu does.

The game itself must consume both requests and report the native delegation
path's successful arm/cancel responses. The test never edits match results,
pretends a manager was hired, or touches a user's existing career.

This is a command round-trip, NOT proof that a delegated match reaches full
time or that a manager contract was signed. Those need separate Mac gates.
"""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "fixtures"))
sys.path.insert(0, str(HERE.parent / "smoke"))
import fixtures  # noqa: E402
import smoke_run  # noqa: E402

MAX_SECONDS = 240
MAX_SETUP_SECONDS = 165
FIRST_CLICK_MS = 12000
SECOND_CLICK_MS = 29000
QUIT_MS = 53000


def read_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError):
        return None


def steps_for_arsenal(user: Path) -> list[str]:
    steps = [step for step in smoke_run.STEPS if " dump " not in step]
    steps = steps[:steps.index("wait first-news") + 1]
    # The pauses allow this test to write commands while CM is waiting for
    # the human manager's Next/Continue click. No manipulation of game memory.
    return steps + [
        f"{FIRST_CLICK_MS} click 45 115",
        f"{SECOND_CLICK_MS} click 45 115",
        f"{QUIT_MS} quit",
    ]


def run_case(program: str, game: str, work: Path) -> None:
    user = work / "user"
    user.mkdir(parents=True, exist_ok=True)
    fixtures.write_settings(str(user), "classic", [
        ("faster_days", "on"),
        ("changing_rules", "off"),
    ])
    steps = smoke_run.scaled_steps(
        "classic", str(user), steps_for_arsenal(user)
    )
    env = dict(
        os.environ,
        HOME=str(work / "home"),
        SDL_VIDEODRIVER="dummy",
        SDL_AUDIODRIVER="dummy",
        RECOMP_NO_BOX="1",
        RECOMP_USER_DIR=str(user),
        RECOMP_FIXED_CLOCK="1",
        RECOMP_SCRIPT="; ".join(steps),
    )
    (work / "home").mkdir(parents=True, exist_ok=True)
    state_path = user / "owner-game-state.json"
    command_path = user / "owner-game-command.txt"
    result_path = user / "owner-game-result.json"
    log_path = work / "native-stderr.log"
    started = time.monotonic()
    sent_arm = False
    sent_cancel = False
    first_result = None

    with log_path.open("wb") as logfile:
        proc = subprocess.Popen(
            [program, "--game", game, "-seed", "1"], env=env,
            stdout=subprocess.DEVNULL, stderr=logfile,
        )
        try:
            while proc.poll() is None:
                elapsed = time.monotonic() - started
                if elapsed > MAX_SECONDS:
                    raise AssertionError(
                        f"Native test exceeded {MAX_SECONDS}s"
                    )
                state = read_json(state_path)
                if not sent_arm and isinstance(state, dict) and state.get("ready"):
                    if state.get("club_name") != "Arsenal":
                        raise AssertionError(f"Wrong test club: {state}")
                    session = state.get("session")
                    club = state.get("club_id")
                    if (not isinstance(session, str) or len(session) != 16
                            or not isinstance(club, int) or club < 0):
                        raise AssertionError("Invalid native bridge session")
                    # Authorised request, written while the native game is
                    # paused on the human manager's first news screen.
                    if command_path.exists():
                        raise AssertionError("Unexpected previous owner command")
                    command_path.write_text(
                        f"delegate_match {session} {club}\n", encoding="ascii"
                    )
                    sent_arm = True
                    print(f"CHECK authenticated native delegate request club={club}",
                          flush=True)

                result = read_json(result_path)
                if sent_arm and not sent_cancel and isinstance(result, dict):
                    if result.get("message") == "native next-match delegation armed":
                        if result.get("status") != "applied":
                            raise AssertionError(f"Arm was not applied: {result}")
                        if command_path.exists():
                            raise AssertionError("Consumed command was not deleted")
                        first_result = result
                        command_path.write_text(
                            f"cancel_delegation {session} {club}\n",
                            encoding="ascii",
                        )
                        sent_cancel = True
                        print("CHECK native manager delegation armed; cancel queued",
                              flush=True)
                    elif result.get("status") == "rejected":
                        raise AssertionError(f"Native game rejected delegate: {result}")
                time.sleep(0.15)
            code = proc.returncode
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=8)
    log = log_path.read_text(encoding="utf-8", errors="replace")
    if code != 0 or "recomp: fatal" in log.lower():
        raise AssertionError(
            f"Native game exit {code}\n{log[-4000:]}"
        )
    if not sent_arm or not sent_cancel or not first_result:
        raise AssertionError(
            f"Native request not completed: arm={sent_arm}, cancel={sent_cancel}\n"
            + log[-4000:]
        )
    if f"owner-game: delegate_match club={club} result=1" not in log:
        raise AssertionError("No native successful arm evidence\n" + log[-4000:])
    if f"owner-game: cancel_delegation club={club} result=1" not in log:
        raise AssertionError(
            "No native successful cancellation evidence\n" + log[-4000:]
        )
    final = read_json(result_path)
    if not isinstance(final, dict) or final.get("status") != "applied" or (
        final.get("message") != "delegation cancelled before kick-off"
    ):
        raise AssertionError(f"Native cancellation not confirmed: {final}")
    if command_path.exists():
        raise AssertionError("Cancelled command was not consumed exactly once")
    if first_result.get("before") != final.get("before") or (
        first_result.get("after") != final.get("after")
    ):
        raise AssertionError("Delegation should not change club cash")
    print("PASS native one-match delegation armed and cancelled before kick-off",
          flush=True)
    print("PASS two authenticated commands consumed once with unchanged club cash",
          flush=True)


def main(args: list[str]) -> int:
    if len(args) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, args[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        raise AssertionError("Missing native executable or packaged game folder")
    with tempfile.TemporaryDirectory(prefix="owner-game-manager-desk-native-") as temp:
        run_case(program, game, Path(temp))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, subprocess.SubprocessError) as e:
        print(f"FAIL native Manager Desk: {e}", file=sys.stderr)
        sys.exit(1)
