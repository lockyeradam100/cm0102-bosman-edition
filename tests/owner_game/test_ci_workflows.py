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

assert "owner-game-active-dev" in fast, "active development must get fast CI"
assert "owner-game-active-dev" not in mac, "dev pushes must not rebuild universal Mac binary"
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
    "Prove contract trace does not change native football saves",
    "Manager Desk native market observer must preserve football saves",
    "Verify native submitted offer survives reload and pending readback",
    "Verify delayed native contract resolution and second reload",
    "Save Mac build",
):
    assert gate in mac, f"Mac regression gate missing: {gate}"
print("PASS active-dev fast CI, integration-only Mac build, caching and all native gates")
