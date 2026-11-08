# Owner Game — Integrated Delivery Plan and Release Gates
**Principle:** design the whole connected game once; deliver functioning vertical systems in controlled, measurable stages. No feature-by-feature game redesign.  
**Date:** 8 November 2026. No calendar promises until build duration, team capacity and technical risk are measured.

## 0. Programme rules
- Work from [MASTER_BLUEPRINT.md](MASTER_BLUEPRINT.md), [ARCHITECTURE.md](ARCHITECTURE.md), [CODEBASE_AUDIT.md](CODEBASE_AUDIT.md), and the [decision register](DECISION_REGISTER.md).
- **Never call a single window/button a completed system.** Every release includes data model, rules, UI, adapter, valid/invalid actions, AI interaction, event history, save/reload, regression tests, Mac packaging.
- Keep a known-good reproducible Mac build and one **backed-up Arsenal test save**. Use branch-per-stage and compare results to the last green checkpoint.
- No open-ended coding based on chat-level guesses; newly discovered design issues are added to the blueprint and assessed for downstream effects before implementation.
- Football completeness is the first release-quality gate. Business mini-world expansion is frozen until the football layer meets it.
- Publish a short release scorecard per build: functionality PASS/FAIL; existing saved-game compatibility PASS/FAIL; financial reconciliation PASS/FAIL; football parity PASS/FAIL; Mac smoke PASS/FAIL; known defects.
- A successful GitHub Actions build is only compilation/packaging evidence unless it specifically ran packaged-app and gameplay smoke tests.

## A. Programme map
| Stage | Complete connected outcome | Dependencies | Exit gate |
|---|---|---|---|
| **P0 — Freeze & audit** | Working baseline archived; football and bridge coverage mapped; feature contracts approved | Current Mac test, source inspection | Reproducible clean Mac build; save backup; documented missing football features, hooks and rights risks |
| **P1 — Football foundation** | CM01/02-complete management and background football world, stable contracts and one-season simulation | P0 | Full football parity checklist, pending negotiation fixed, complete one-season playthrough, no missing actions |
| **P2 — Shared game core** | Game identity, reliable clock/events, authority, versioned world store, atomic career save + migrations | P0, P1 football hooks | READ→IDENTIFY→INFLUENCE→CONTINUE→READBACK→PERSIST tests; one-save/other-save isolation and crash recovery |
| **P3 — Owner finances & acquisition** | Owner wallet + club control/ownership + true equity/loan posting + balance reporting | P2; CM finance adapter | Owner £5m funds *deducted*, club funds received *once*, loans/equity correct; save/reload; non-funded attempt rejected |
| **P4 — Boardroom/manager lifecycle** | Recruit, poach, appoint, support, discipline, dismiss/replace manager under charter | P1/P2/P3 | Lifecycle end-to-end including compensation, manager delegation, reputation, staff continuity |
| **P5 — Matchday & owner visibility** | Attend home/away, skip/instant result, control as manager optionally, supporter perception | P1/P2/P4 | Same CM match engine; delegated result + aftermath; no forced viewing; attendance effects persist |
| **P6 — Complete owner career loop** | One club and multi-season owner story on Mac; coherent inbox, strategic dashboard and season-end | P1–P5 | 3 seasons without game-breaking action; acquisitions, finances, board, football, delegation, save consistency |
| **P7 — Historic ~1980 world** | Era-accurate 1980 start and longitudinal rules/economy | P1–P6 + researched data | Verified database/rule provenance, season transitions, economy scaling and historical tests |
| **P8 — Deepening and public readiness** | Academy/stadium/commercial/property and rival-owner sophistication; distribution/legal readiness | P6/P7 | Content/rights gate, realistic multi-decade AI, profiling, UX, reliability, released source obligations addressed |

**Parallel work is permitted** only where contracts are stable: historical data research, UI mockups and testing tooling can proceed during P1/P2, but no incompatible second finance/state engine. P2 infrastructure can progress alongside P1 audits; do not merge it into gameplay before football contracts are proven.

## B. P0 — the immediate action list (no user decisions required)
**Deliverables**
1. Freeze working prototype commit `a06e98d38094584ac5270b329ff423c16df5d9af` and record successful real Mac demonstration (Arsenal, 14 July 2001, £27,951,510 visible, owner bridge connected).
2. Inventory hooks, hardcoded proof experiments, binary builds and packaging pipelines; separate prototype-only commands from shippable features.
3. Verify clean Mac build and preserve game save in `~/Library/Application Support/cm0102`; no deletion/migration scripts run on user machine.
4. Establish a canonical backlog of **integrated capabilities**, prioritised by dependencies, not speculative hundreds of isolated buttons.
5. Audit licensing and distribution, including copyrighted game assets, historical databases, real-world names/trademarks and GPLv3 source obligations, before any external/commercial distribution.
6. Define high-value automated seed save fixtures and build smoke suite; instrument missing runtime behaviour before large changes.

**Exit**: another developer can reproduce a build and identify current behaviour vs design without reading chat transcripts.

## C. P1 — football reality test matrix
Create a capability matrix with one row per required operation and columns: `Exists in native kernel?`, `Player can operate?`, `Delegation route?`, `AI operates?`, `Persists?`, `UI accessible?`, `Regression test?`.

**Required groups:**  
(1) squads and staff; (2) formation/tactics/captaincy/set-pieces; (3) scouting/transfers/fees/loans/agents/free agents; (4) player/staff offers, renewal, release, compensation, timeouts; (5) training, injuries, bans, permits and registration; (6) youth/reserves/pathways; (7) domestic/foreign/background leagues, cups, European/international competitions; (8) calendar/preseason/friendlies, referees/weather; (9) results/live table/media/news/reactions; (10) football budgets/board pressure; (11) save/continue/multi-season transition.

**Blocking P1 defect from current experiments:** contract offers may fail to persist in native `ContractManager` after submit. The exact Figo → Aston Villa test proves a screen could be opened but **not** that a pending offer remained live. Do not conclude the player rejected or the negotiation worked. Fix lifecycle for ordinary candidates and verify on restart.

**Required P1 scenarios:**
- Start Arsenal 2001, create a staff/player offer, continue over expected response period, observe pending → reply/timeout, save/reload mid-pending, exercise accepted and rejected outcomes.
- Apply transfer, loans, player release, contract renewal, staff search and hiring; ensure accounting/budgets and squads update.
- Delegate training/press/non-critical staff tasks and complete a season with no forced routine work.
- Simulate background English and foreign competitions while player club advances; verify table, rules, European qualification, registration, world news.
- Run an unmodified CM reference save in parallel and compare invariants and explicit deviations over a full season.
- Simulate 3, 10 and (later) 15 seasons for club histories, aging, AI transfers, managerial careers, youth throughput and competition integrity.

**Gate:** zero missing core football action, zero unresolved financial/contract corruption, zero forced match-viewing, football entertaining enough to play solely as manager.

## D. P2 — reliable foundation
Implementation work packages:
- `core/ids`: game UUID, owner/club/person stable IDs and CM reference mapping.
- `core/clock`: exactly-once day/turn/match event ordering.
- `core/commands`: validated command bus, authority, versioning and idempotency.
- `core/events`: append-only event journal and read-model projection.
- `core/save`: owner DB, football save checkpoint linkage, rollback recovery and migrations.
- `integrations/cm`: typed contract with native reads/writes; read-only probes and trace output before writes.
- `ui/shell`: Owner Home, inbox and concise decision panels, not hardcoded F12 source text.

**Gate:** save ten times and reload through different careers; no bleed of owner state, no duplicate transaction; queued decisions complete exactly once; real packaged Mac test passes.

## E. P3 — integrated finance and acquisition
First fully integrated business-facing vertical slice:
```text
Starting personal cash £100m + Arsenal football cash £27,951,510
→ owner buys controlling interest at agreed price / scenario grant
→ owner sets a £5m investment as interest-free shareholder loan
→ confirm owner available funds and authority
→ debit owner bank £5m / create owner loan receivable £5m
→ credit club bank £5m / create club loan liability £5m
→ verify native Arsenal cash £32,951,510
→ show combined dashboard and statement of changes
→ save, quit, reload (both balances, debt and history unchanged)
→ repay £1m from club if liquidity/covenants allow; loans fall to £4m
```
Equivalent equity path has **no debt**, dilution/holding accounting if needed. Invalid path: insufficient owner cash must be rejected with zero club effect. Repeated submission must not inject a second £5m. Switching clubs mid-command must reject safely. Interest accrual and repayments must pass a full season.

**Gate:** trial-balance, bank, ownership and CM cash reconciliation all green across every scenario; no silent discrepancy.

## F. P4 — manager hiring, poaching and support
**Scenario:** owner appoints a manager on a realistic contract, agrees wage/transfer discretion and a club plan, delegates first-team responsibilities, assesses performance, challenges unmet targets, grants/rejects an exception, approaches a manager under contract elsewhere (permissions/discreet approach/compensation), appoints successor and pays severance. All changes survive save/reload and affect supporters, players and the market.

**Gate:** real native manager/staff links and actions; manager drives football under authority; cannot fake replacement by merely changing a UI string.

## G. P5 — matchday choice
**Scenario A:** owner does not attend Arsenal home fixture → manager controls selection/tactics → CM simulates → instant result and readable report → league, fitness, morale, finance and owner visibility update.  
**B:** owner attends away fixture → same result mechanics, different attendance/relationship event; no guaranteed positive result.  
**C:** owner-manager runs match manually → no conflicting delegation.  
**D:** delegate then cancel before lock → no duplicate match or stuck holiday; control returns after fixture.

**Gate:** 20-match season sample with mixed attendance; no obligatory viewing; same fixture not simulated twice; supporter perception evolves but match engine parity is maintained.

## H. P6 — the integrated playable game gate
- New game with owner identity, rational starting wealth and a club under control or acquired.
- Dashboard, inbox, control charters, player/manager staffing, contract/transfer cycle, finance and debt.
- Meaningful owner decisions and meaningful delegated days, instant results, season end and change of season.
- 3 completed seasons with simulated AI club behaviour and meaningful consequences, not scripted test-only responses.
- Save in one Mac session, exit, replace application with next compatible build, reopen, continue another season.
- A useful first-time player tutorial and contextual explanations; experienced CM player can skip it.

**If P6 fails:** it remains an experimental build, even if the installer is beautiful.

## I. P7 — 1980 historical start gate
- Historically accurate start around **1980** is the defining release target. Use 1980/81 as the default scenario unless validated historical source availability dictates a documented equivalent 1980s year; don't silently substitute a 2001 save with old date text.
- Club and national rules, age-correct squads, period economics, sponsorship, stadium conditions and competitions.
- Era-to-era rules tested with changes in decades, European structures, transfer regimes and club finance models.
- Simulated alternative history carries forward as truth; real future history is reference/context only.
- Historical accuracy review and clearly labelled provenance/estimates for incomplete 1980 records.

**Gate:** complete a test career from 1980 across major regime changes, with finances and football functioning throughout.

## J. P8 and deeper worlds
Extend the mature shared model with stadium planning/financing, youth pipeline, detailed sponsorship/revenue, property/business mini-worlds, multi-club governance, public markets, hostile takeovers, investor syndicates and richer rival-owner simulation. Every new world must justify its inclusion through owner strategies and connected cash, risk and consequences. Each adds autonomous AI, save support and test gates.

## K. Release confidence report (required for every release candidate)
| Evidence | Required test |
|---|---|
| Build | Mac universal app compiles, packages, launches from extracted ZIP; Windows/Linux as targeted |
| Save | Fresh and migrated saves, crash rollback, separate career isolation |
| Football | Full-season loop, contracts pending persistence, staff, transfers, youth, background world |
| Owner | Identity, shares, managers, delegated roles, approvals and history |
| Money | Opening balance, double-entry posting, debt/equity, CM cash delta and reconciliation |
| Match | Attendance vs instant delegation, normal CM outcome, return of control |
| Historical | Accurate dated data/rules with explicit year-support list |
| Reliability | Stress/multi-year, deterministic regression, error recovery and performance |
| Distribution | GPL/copyright/licence assessment and release provenance |
| User validation | Mac owner test save reaches expected screen and follows an actual end-to-end decision |

## L. Backlog discipline
Use epic-sized tickets based on integrated capabilities, **not** "add a button", "change a modal", or "inject £5m":
- `OG-FOOTBALL-COMPLETE` (P1)
- `OG-SAVE-CORE` (P2)
- `OG-AUTHORITY` (P2/P4)
- `OG-OWNER-FINANCE` (P3)
- `OG-MANAGER-LIFECYCLE` (P4)
- `OG-MATCHDAY` (P5)
- `OG-CAREER-LOOP` (P6)
- `OG-HISTORICAL-1980` (P7)
- `OG-BUSINESS-WORLDS` (P8)

Each epic is split into implementation tasks only after the architecture and acceptance tests exist. Incomplete requirements do not get papered over with demo labels. Apply the **product acceptance gates before progressing**.
