# Agent development rules — Owner Game

## Source of truth
Read these documents **before proposing or changing Owner Game code**:
- `docs/owner-game/README.md`
- `docs/owner-game/MASTER_BLUEPRINT.md`
- `docs/owner-game/ARCHITECTURE.md`
- `docs/owner-game/DELIVERY_PLAN.md`
- `docs/owner-game/CODEBASE_AUDIT.md`
- `docs/owner-game/DECISION_REGISTER.md`
- `docs/owner-game/FOOTBALL_PARITY_AUDIT.md`
- `docs/owner-game/IMPLEMENTATION_BACKLOG.md`

## Strict product invariants
1. Owner Game is an **owner-first historical football business sandbox** with CM01/02-complete football as a mandatory foundation.
2. Don't build isolated buttons/modal overlays without the full domain lifecycle: entity, authority, state, action, effect, event, persistence, UI, tests.
3. Bosman is the **authoritative football kernel** unless a documented proof shows it is unsuitable. Native CM results and player agency must remain intact unless a tested deliberate override is specified.
4. An owner has **personal cash and assets separate from the club's cash**. No uncoupled `inject_cash` is a legitimate production finance transaction. Equity and shareholder loans have different ledger entries and obligations.
5. Owner/player/manager authority is separate; the manager manages on the owner's behalf. Owner can attend or skip any match; instant manager result is mandatory and saves correctly.
6. Contract offers can be pending: no response is not a rejection. The Figo/Aston Villa proof is an unresolved persistence issue, not a reusable player-specific game mechanic.
7. One clock, stable career identifiers, explicit commands/decisions, exact-once postings, and atomic ownership+football saves. No cross-career shared JSON state.
8. Historical **1980/81** start is a researched release target, **not** simulated by relabelling 2001/02.
9. Preserve working Mac gameplay and saved-game compatibility; an Actions green compile is not a proof that an Owner Game feature works.
10. Football P1 completeness gate must be passed before extensive optional property/commercial minigames are built.

## Before touching code
- Identify current stage and epic from `IMPLEMENTATION_BACKLOG.md`; don't pick a random desirable feature.
- State an integrated use case and tests *before* implementing.
- Inspect all related systems and native functions to avoid duplicate or conflicting authority.
- Document game-world/finance/save contract changes and any historical-era assumptions.
- Keep an unmodified reference path for CM parity and a known-good Mac build.

## After changing code
- Add tests for success, rejection, duplicate submission, delayed response, cross-save isolation, save/reload, restart and affected football AI.
- Test the actual packaged Mac executable and a real save (not only compilation).
- Compare native football behaviour with a fixed baseline when hooks change.
- For money, require balanced ledger postings, funded source and verified CM cash delta.
- For matchday, require optional owner attendance and manager simulation with no double-result or loss on reload.
- For offers, require native pending response and history; never report unproven success.
- Update the backlog evidence and codebase audit with commit/test/run; mark done only if the release gate passed.

## Do not
- Declare an unfinished prototype as the finished product.
- Hardcode Figo/club IDs or free £5m as generic gameplay.
- Delete or overwrite the user's Arsenal save.
- Ask the user to make routine architectural decisions already settled in the decision register.
- Assume ownership and Bosman football state can be saved independently without a consistency protocol.
- Promise commercial distribution of original game assets without permission/licence review.

## Scope of these rules
These additional rules apply to Owner Game extension work on top of the Bosman port. Core upstream bug fixes should still preserve the native game's existing tests and licensing notices.
