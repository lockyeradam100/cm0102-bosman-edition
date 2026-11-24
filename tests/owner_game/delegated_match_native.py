#!/usr/bin/env python3
"""Mac integration: prove one real CM delegated fixture finishes and returns control.

This is NOT an arm/cancel smoke. The only success condition is CM's native
fixture result observed by the game itself, followed by native holiday OFF.
The isolated Arsenal career never touches the user's real saves. If the game
does not reach a result, this gate FAILS rather than inventing a match.
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

TIMEOUT = 300


def snapshot(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def test(program, game, root):
    user = root / "user"
    user.mkdir()
    (root / "home").mkdir()
    fixtures.write_settings(str(user), "classic", [
        ("faster_days", "on"), ("changing_rules", "off")
    ])
    steps = [s for s in smoke_run.STEPS if " dump " not in s]
    steps = steps[:steps.index("wait first-news") + 1]
    # Allow the initial safe point to consume our authenticated command,
    # then keep progressing the real game. Native holiday owns the fixture.
    # Real CM can remain on the news screen after clicking Continue. Use
    # the native Space-to-Continue shortcut (the same tested input form as
    # smoke_run.MATCH_STEPS), allowing queued dialogue to finish.
    # This is UI input, never a fabricated fixture result or a memory edit.
    # The 2001 Arsenal seed reaches the Henry transfer bid on 16 July.
    # It is a real blocking native news decision, not a stalled simulation.
    # Answer it using the established CM smoke-match controls before
    # waiting for the one-match holiday to take over. The previous blind
    # Space/click loop could never answer the Respond/Reject/Yes dialogue.
    # Do not alter the bid, fixture, game clock or match outcome in memory.
    steps += [
        # A *fresh* career opens on 14 July, unlike the older smoke test's
        # prematch save. Explicitly continue it to the bid; don't click
        # Respond while still on the first news screen on 14 July.
        "1500 click 45 115",            # Continue / next unread
        "4000 click 45 115",
        "7000 click 45 115",
        "10000 click 45 115",
        "wait day:16.7.01",
        "500 click 172 230",            # Respond to real Henry bid
        "wait screen:0x8e9760",
        "1000 click 502 573",            # Reject (native CM action)
        "wait screen:0x8efda0",
        "1000 click 705 573",            # Confirm rejection
        "wait news",
        "500 press space",
        "wait day:18.7.01",
        "500 press space",
        "1500 move 400 30",
        "wait continue",
        "500 press space",
        # Several days before the fixture, the armed native delegation
        # should enter CM holiday. Never press Kick Off manually here:
        # the manager must run the genuine CM simulation.
    ]
    for ms in (12000, 24000, 36000, 48000, 60000, 72000,
               84000, 96000, 108000, 120000, 132000, 144000):
        steps.append(f"{ms} press space")
    steps += ["180000 quit"]
    env = dict(os.environ, HOME=str(root / "home"),
               SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy",
               RECOMP_NO_BOX="1", RECOMP_USER_DIR=str(user),
               RECOMP_FIXED_CLOCK="1",
               RECOMP_SCRIPT="; ".join(
                   smoke_run.scaled_steps("classic", str(user), steps)))
    state_path = user / "owner-game-state.json"
    command_path = user / "owner-game-command.txt"
    result_path = user / "owner-game-result.json"
    log_path = root / "native-stderr.log"
    started = time.monotonic()
    queued = False
    applied = False
    completed = False
    completion_seen_at = None
    confirmed_stable = False
    start_date = None
    end_date = None
    observed_dates = []
    observed_phases = []
    last_progress = time.monotonic()
    last_date = None
    last_phase = None
    with log_path.open("wb") as log:
        proc = subprocess.Popen([program, "--game", game, "-seed", "1"],
                                env=env, stdout=subprocess.DEVNULL, stderr=log)
        try:
            while proc.poll() is None:
                if time.monotonic() - started > TIMEOUT:
                    raise AssertionError("native fixture exceeded 300 seconds")
                state = snapshot(state_path)
                if state and state.get("ready"):
                    date = state.get("date")
                    phase = state.get("delegation_status")
                    if date and (not observed_dates or observed_dates[-1] != date):
                        observed_dates.append(date)
                        last_progress = time.monotonic()
                        last_date = date
                    if phase is not None and (not observed_phases or observed_phases[-1] != phase):
                        observed_phases.append(phase)
                        last_progress = time.monotonic()
                        last_phase = phase
                if state and state.get("ready") and not queued:
                    if state.get("club_name") != "Arsenal":
                        raise AssertionError(f"wrong club: {state}")
                    session = state.get("session")
                    club = state.get("club_id")
                    if not isinstance(session, str) or len(session) != 16:
                        raise AssertionError("invalid session token")
                    if not isinstance(club, int) or club < 0:
                        raise AssertionError("invalid club")
                    start_date = state.get("date")
                    command_path.write_text(
                        f"delegate_match {session} {club}\n", encoding="ascii")
                    queued = True
                    print("CHECK native Arsenal delegation requested", flush=True)
                result = snapshot(result_path)
                if result and result.get("message") == "native next-match delegation armed":
                    if result.get("status") != "applied":
                        raise AssertionError(f"delegation not applied: {result}")
                    applied = True
                if state and state.get("delegated_match_completed") is True:
                    if not applied:
                        raise AssertionError("native completion appeared before accepted delegation")
                    if state.get("delegation_status") != 0:
                        raise AssertionError(f"CM did not return control: {state}")
                    if not completed:
                        completed = True
                        completion_seen_at = time.monotonic()
                        end_date = state.get("date")
                        print(f"CHECK native delegated fixture completed at {end_date}",
                              flush=True)
                    # The game must remain alive with the restored human
                    # state, not merely report completion in one transient
                    # JSON snapshot during its holiday teardown.
                    if time.monotonic() - completion_seen_at >= 2.0:
                        confirmed_stable = True
                        break
                elif completed:
                    raise AssertionError("completed fixture readback disappeared before stable human return")
                # A game stuck at a native confirmation or news decision must
                # fail with useful evidence rather than being misreported as
                # a successful delegated match. Do not mutate the football
                # save, fixture or match result to force progress.
                if applied and not completed and time.monotonic() - last_progress > 85:
                    raise AssertionError(
                        f"native match stalled: date={last_date} phase={last_phase} "
                        f"dates={observed_dates} phases={observed_phases}"
                    )
                time.sleep(0.2)
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=8)
    log = log_path.read_text(encoding="utf-8", errors="replace")
    if not queued or not applied or not completed or not confirmed_stable:
        raise AssertionError(
            f"native match incomplete: queued={queued} applied={applied} "
            f"completed={completed} stable={confirmed_stable} "
            f"dates={observed_dates} phases={observed_phases}\n" + log[-5000:])
    if "owner-game: manager takes the next match" not in log:
        raise AssertionError("no native CM holiday delegation start")
    if log.count("owner-game: delegated match complete; control returned") != 1:
        raise AssertionError("fixture completion was not recorded exactly once")
    if "owner-game: delegated match complete; control returned" not in log:
        raise AssertionError("no native result and human control return")
    if not end_date or end_date <= start_date:
        raise AssertionError(f"fixture date did not advance: {start_date} -> {end_date}")
    if command_path.exists():
        raise AssertionError("delegation command was not consumed")
    print("PASS CM played real delegated fixture and restored human control",
          flush=True)


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    program, game = map(os.path.abspath, argv[1:])
    if not os.path.isfile(program) or not os.path.isdir(game):
        raise AssertionError("missing native Mac executable or game Data")
    with tempfile.TemporaryDirectory(prefix="owner-game-native-match-") as tmp:
        test(program, game, Path(tmp))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv))
    except (AssertionError, OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"FAIL native delegated match: {error}", file=sys.stderr)
        sys.exit(1)
