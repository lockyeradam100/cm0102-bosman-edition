# Owner Game — Single Source of Truth

**8 November 2026.** These documents supersede ad-hoc feature-by-feature plans in earlier prototype conversations. They are a structured design and delivery baseline, **not** a claim that these features already exist in the executable.

1. [MASTER_BLUEPRINT.md](MASTER_BLUEPRINT.md) — what the game must become, its roles, football completeness, financial logic, historical world and player experience.
2. [ARCHITECTURE.md](ARCHITECTURE.md) — what owns each piece of state, the shared clock, commands, authority, ledger, integration boundary and atomic save model.
3. [DELIVERY_PLAN.md](DELIVERY_PLAN.md) — stages P0–P8, their dependencies, integrated acceptance gates and the football-first release policy.
4. [CODEBASE_AUDIT.md](CODEBASE_AUDIT.md) — current implementation evidence, known limitations and high-risk blockers.
5. [DECISION_REGISTER.md](DECISION_REGISTER.md) — settled choices, technical defaults and real unknowns.

## Current status
- **Mac integration:** Arsenal owner status and F12 Overview have been observed working in the packaged app.
- **Current code:** one-off £5m cash injection, matchday delegation prototype and targeted contract investigation exist. None constitutes finished owner finance, matchday visibility or contract persistence.
- **Baseline commit:** `a06e98d38094584ac5270b329ff423c16df5d9af`.
- **Next execution gate:** P0 codebase/integration audit, then P1 complete-football parity audit and fix before adding optional property/commercial worlds.
- **Mac save protection:** existing careers normally reside in `~/Library/Application Support/cm0102`. Do not delete/overwrite saved game data when testing new builds.

## The delivery rule
A feature does not graduate from experimental to done until it has **real end-to-end state, authority, consequence, history, save/reload compatibility, gameplay tests and packaged-Mac verification**. A green compiler or a nice screen alone is not a gameplay completion milestone.

## Product charter in one line
**Own the club. Hire the people. Decide the strategy. Choose the football role. Build the institution. Live with the consequences across football history.**
