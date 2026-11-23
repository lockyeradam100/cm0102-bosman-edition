#!/usr/bin/env python3
"""Check the fast-development / release-build split before an expensive Mac run.

The active development branch runs source checks on every push; only the
integration branch builds the universal native Mac app. SDL2 and compiler
outputs must be restored from cache without dropping any game parity gates.
"""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
mac = (root / ".github/workflows/owner-game-mac-build.yml").read_text()
fast = (root / ".github/workflows/owner-game-fast-source-checks.yml").read_text()

# A one-space indentation error in a native Mac test step once stopped the
# entire GitHub workflow before any jobs could run. Enforce the GitHub Actions
# step list indentation without introducing another Python dependency.
steps = [(number, line) for number, line in enumerate(mac.splitlines(), 1)
         if line.lstrip().startswith("- name:")]
assert steps, "Mac integration workflow contains no named steps"
for number, line in steps:
    assert line.startswith("      - name: "), (
        f"Mac CI workflow line {number}: invalid step indentation {line!r}"
    )
assert "tests/owner_game/manager_desk_native_commands.py" in fast

assert "owner-game-active-dev" in fast, "active development must get fast CI"
assert "owner-game-active-dev" not in mac, "dev pushes must not rebuild universal Mac binary"
release_guide = root / "docs/owner-game/HOLIDAY_EDITION_2026-11-24.md"
assert release_guide.is_file(), "Holiday Edition installation and acceptance guide missing"
guide = release_guide.read_text()
assert "24 November 2026" in guide
assert "43899286537" in guide, "last verified Mac fallback must be explicit"
assert "Application Support/cm0102" in guide, "user save backup guidance absent"
assert "Assemble playable Owner Game Mac beta" in mac
assert "name: Owner-Game-Holiday-Playable-Mac" in mac
assert "Publish playable Mac beta before experimental full-match gate" in mac
assert mac.index("Publish playable Mac beta before experimental full-match gate") < mac.index("Prove delegated match reaches native result and returns human control")
assert "Assemble Owner Game Holiday Mac download" in mac
assert "name: Owner-Game-Holiday-Mac" in mac
assert "build-owner-mac/holiday-release/*" in mac
assert "READ-ME-FIRST.txt" in mac
assert "tests/owner_game/delegated_match_native.py" in mac
assert "test \"$(find build-owner-mac/holiday-release" in mac, "single-package guard missing"
assert "Manager Desk owner command must arm and cancel native one-match delegation" in mac
assert "owner-game-offer-roundtrip" in mac, "release integration branch is missing"
assert "workflow_dispatch:" in mac, "manual Mac integration should remain possible"
assert "sdl2-cache" in mac
assert mac.count("if: steps.sdl2-cache.outputs.cache-hit != 'true'") == 2
assert "sdl2-arm64" in mac and "sdl2-x86_64" in mac
assert "CMAKE_C_COMPILER_LAUNCHER: ccache" in mac
assert "CCACHE_DIR:" in mac
assert "ccache-v1-" in mac
for gate in (
    "Owner Game real Arsenal save and reload smoke",
    "Manager Desk owner command must arm and cancel native one-match delegation",
    "Prove delegated match reaches native result and returns human control",
    "Prove contract trace does not change native football saves",
    "Manager Desk native market observer must preserve football saves",
    "Verify native submitted offer survives reload and pending readback",
    "Verify delayed native contract resolution and second reload",
    "Save Mac build",
):
    assert gate in mac, f"Mac regression gate missing: {gate}"
print("PASS active-dev fast CI, integration-only Mac build, caching and all native gates")
