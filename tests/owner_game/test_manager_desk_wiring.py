#!/usr/bin/env python3
"""Manager Desk action wiring: native human-turn action, no SDL-side mutation.

Fast source gate. Real match simulation and save/reload still need Mac
integration; this test does not claim to prove gameplay by itself.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
bridge = (root / "src/recomp_rt/win/owner_game.c").read_text()
ui = (root / "src/recomp_rt/win/user32.c").read_text()
native = (root / "src/recomp_rt/win/holiday_next.c").read_text()
smoke = (root / "tests/owner_game/bridge_smoke.py").read_text()

assert '"delegate_match" : "cancel_delegation"' in ui, "missing owner-facing delegation/cancel commands"
assert 'owner_game_manager_desk(state, club, date, session, club_id)' in ui
assert "if (k == SDLK_o && (mod & KMOD_SHIFT)" in ui, "Mac shortcut absent"
assert 'native_manager_name' in bridge
assert 'controller_name' in bridge
assert "native_manager_id" in smoke and "controller_name" in smoke
assert 'strcmp(action, "delegate_match") == 0' in bridge
assert "((is_delegation || is_cancel) && fields != 3)" in bridge
assert "if ((uint64_t)session != og_session)" in bridge
assert "if (club_id != s.club_id)" in bridge
assert 'host_unlink(path);' in bridge, "one-shot commands must not be replayed"
assert "owner_game_delegate_next_from_safe_point();" in bridge
assert "owner_game_cancel_delegation_from_safe_point();" in bridge
assert 'strcmp(action, "cancel_delegation") == 0' in bridge
assert 'result == -3 ? "next match already delegated"' in bridge
assert "int owner_game_delegate_next_from_safe_point(void)" in native
assert "return manager ? delegate_arm(manager) : -1;" in native
assert "if (delegate_context_matches(slot, manager)) return -3;" in native
assert "if (delegated_started[slot]) return -2;" in native
assert "delegate_restore_and_clear(slot, manager, 0);" in native
assert 'int owner_game_delegation_status(void)' in native
assert 'delegate_context_matches(slot, manager)' in native
assert 'int delegation_status = owner_game_delegation_status();' in bridge
assert 'delegation_status == 2 ? "Manager handling fixture"' in ui
assert 'chosen == 1 && delegation_status != 0' in ui
assert 'chosen == 2 && delegation_status != 1' in ui
assert '"Cancel Delegation"' in ui
assert "if (match <= today + 1) delegate_start(slot, manager);" in native
assert "set_holiday(manager, 1);" in native
assert "delegate_restore_and_clear(" in native
# Sidebar and command-file cancellation must share one identity-checked transition.
sidebar = native[native.index("if (id == DELEGATE_MATCH_ID)"):native.index("/* The holiday setter")]
assert "owner_game_cancel_delegation_from_safe_point();" in sidebar
assert "delegate_restore_and_clear(slot, manager, 0);" not in sidebar, (
    "sidebar must not bypass current-career identity checks"
)
assert "No valid delegation for the current club and manager." in sidebar
assert "o_007615d0();" in native, "native sidebar behavior must remain"
assert "owner_game_delegate_next_from_safe_point" not in ui
assert "rt_callback(" not in ui[ui.index("static void owner_game_manager_desk"):ui.index("static void owner_game_overview")], "SDL input must not mutate native game"
assert "process_command();" in bridge[bridge.index("void owner_game_after_human_turn"):]
print("No match score is computed or written by the Manager Desk.")
print("PASS Manager Desk UI -> process-bound command -> safe-point native delegation")
print("PASS stale session, wrong club, invalid requests guarded; native sidebar retained")
