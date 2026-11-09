# Owner Game — Implementation Backlog and Progress Ledger
**Maintained in GitHub because Issues are disabled in this repository.** This file is the programme tracker until a supported issue-tracking system is available.
**Status date:** 8 Nov 2026. `DONE` requires evidence; `SCOPED` is not implemented.

## Current milestones
| Epic | Stage | State | What is already real | Blocking work |
|---|---|---|---|---|
| OG-00 | P0 Freeze/audit | IN PROGRESS | Mac F12 owner status screenshot, successful prior native universal build, working Arsenal save | reproducibility test, asset/save backup validation, code inventory completion |
| OG-01 | P1 Football completeness | SCOPED | Bosman football kernel; native match/season; known CM-style controls; early manager delegation | contract pending persistence, entire football matrix, multi-season tests |
| OG-02 | P2 Shared state | NOT STARTED | Per-process one-shot text command / JSON readback | save identity, event bus, authority, migration and atomic persistence |
| OG-03 | P3 Owner finance | NOT STARTED | `inject_cash` sandbox proof code | owner wallet, holding, funded journal, equity/loan, reconciliation |
| OG-04 | P4 Manager governance | NOT STARTED | Native staff and manager data in football kernel | appointments/compensation/poaching/board mandate and lifecycle |
| OG-05 | P5 Matchday/attendance | PROTOTYPE | `holiday_next.c` next-match delegation logic | user Mac end-to-end, save state, owner visibility history |
| OG-06 | P6 Career loop | NOT STARTED | Arsenal football career can open/save | 3-season ownership lifecycle with all previous epics |
| OG-07 | P7 Historical 1980 | NOT STARTED | 2001–2025 season-switching base | 1980 data sources, rules, wage/finance economy, tests |
| OG-08 | P8 Deeper worlds | DESIGN ONLY | Prior project vision | commercial, property, stadium, portfolios, AI sophistication after P1/P6 |

## P0 — next executable tasks
- [x] P0-01 Preserve the last working Mac integration commit `a06e98d38094584ac5270b329ff423c16df5d9af` as a recorded baseline.
- [x] P0-02 Create independent `owner-game-master-blueprint` branch, keeping the working code branch unchanged.
- [x] P0-03 Consolidate the vision, module contracts, financial rules, owner/manager relationship and signature 1980 target.
- [x] P0-04 Source review of `owner_game.c`, `user32.c`, `holiday_next.c`, negotiation hooks, human turn and Mac workflows.
- [x] P0-05 Record high-severity gaps and honest evidence statuses in `CODEBASE_AUDIT.md`.
- [ ] P0-06 Reproduce an extracted ZIP install + game launch + Arsenal save on a fresh Mac test environment, collect logs.
- [ ] P0-07 Export/backup a real Arsenal `.sav` as a test fixture (with permission, no copying personal save into public repository).
- [ ] P0-08 Baseline smoke test automated: boot / F12 readback / day advance / save and reload / new career isolation.
- [ ] P0-09 Record exact native hooks and input/output for offers, manager appointment, match delegation, native cash and save boundaries.
- [ ] P0-10 Build and validate detailed football parity evidence matrix, test club and scenarios.
- [ ] P0-11 Review GPLv3, original game, assets, trademarks and historical dataset rights before release planning.
- [ ] P0-12 Record independent performance baseline for Mac cold start, day advance, ten fixture days, save/load and game progression.

## P1 — first engineering and gameplay blockers, ordered
- [ ] P1-01 Reproduce submitted Figo/Aston Villa offer not persisting; classify native `add_contract_offer`, `remove_contract_offer`, `will_negotiate` and final decision path. No forced false rejection.
  - Diagnostic work committed: Figo-only gameplay overrides are now off unless `RECOMP_OWNER_FIGO_PROOF=1`; opt-in `RECOMP_OWNER_CONTRACT_TRACE=1` records native add/remove and the ContractManager daily update's pending-state readback for all human-club offers (commit `b03274187304cbf1b1de6a25c7fcd7ebd64d9dbd`).
  - **Not yet fixed:** reproduce a live submitted offer, establish the cause, verify native pending status after advancing and after save/reload, then write regression tests. No status marked DONE until those pass.
- [ ] P1-02 Fix generic staff/player contract pending lifecycle; check 3 ordinary players, 2 staff, a contracted manager and a free agent in addition to the Figo fixture.
- [ ] P1-03 Complete player transfers, fees, loans, free agency, renewal, release/mutual termination and compensation test scenarios.
- [ ] P1-04 Complete manager/staff search, hire, authority and dismissal test scenarios in native football mode.
- [ ] P1-05 Complete tactics, formations, selection, set pieces, captaincy, training and injury/registration paths with manual/delegated behaviours.
- [ ] P1-06 Test reserve/youth backgrounds, development and promotion pathways, not forced manual reserves.
- [ ] P1-07 Test domestic leagues, cups, foreign/background leagues, Europe, friendlies, referee/weather, rules and world news across complete season.
- [ ] P1-08 Season rollover, aging, contracts, AI spending, promotions/relegations, competition qualifications and 3/10/15-year progression diagnostics.
- [ ] P1-09 Native CM parity comparison on fixed saves/seeds and documented intentional overrides.
- [ ] P1-10 Sign off **complete football-action gate** before optional commercial/property game development.

## P2–P6 integrated epics, immediate acceptance proof
- [ ] P2-01 Career UUID and cross-save isolation test.
- [ ] P2-02 Durable command+event storage, actor authority and exact-once tests.
- [ ] P2-03 Paired football+owner checkpoint, failure injection and migration test.
- [ ] P3-01 Real owner wealth and owner/club shareholding model.
- [ ] P3-02 Double-entry equity and shareholder loan with owner-source deduction and native CM cash reconciliation.
- [ ] P3-03 Interest, repayments, cash availability, insufficient-funds, retries and save/reload.
- [ ] P4-01 Appoint/poach/support/review/dismiss manager with native staff links, compensation and relationship effects.
- [ ] P5-01 Native instant result + attendance toggle and role delegation.
- [ ] P5-02 Save and reload between arming delegation and kick-off, no double match.
- [ ] P5-03 Persistent supporter perception for home/away/absence patterns.
- [ ] P6-01 Complete one club/three season game, show investment → football/manager → supporters → accounts → continuation.
- [ ] P6-02 Mac packaged upgrade and regression sign-off.

## P7 + P8 scoped, not abandoned
- [ ] P7-01 Document provenance and feasibility of genuine 1980/81 season database.
- [ ] P7-02 Historical rules and economic parameters by date.
- [ ] P7-03 1980 start through decades, history divergence and save migration test.
- [ ] P8-01 Academy and first-team youth pathway improvements.
- [ ] P8-02 Stadium, training infrastructure and multi-year capital works.
- [ ] P8-03 Commercial, sponsorship, retail, hospitality and media.
- [ ] P8-04 External businesses/property linked to owner wealth.
- [ ] P8-05 AI owners/portfolios, multi-club governance, takeovers and economic cycles.
- [ ] P8-06 Legal/rights approvals and distribution readiness.

## Definition of DONE for any checked implementation
1. Domain data model and economic/football rules designed.
2. Native engine adapter has verified read/write lifecycle.
3. Correct authority, validation, time and actor decisions implemented.
4. Complete UI and AI/delegation pathways.
5. Error/negative/duplicate/expiry paths.
6. One end-to-end gameplay scenario in a packaged Mac app.
7. Save/reload and version upgrade verified.
8. Football regression, finance reconciliation and relevant stress tests passed.
9. Evidence recorded by commit, test command and screenshot/log where appropriate.

Any status change must link to actual test evidence. A feature cannot be marked `DONE` because a GitHub Actions compilation succeeded.
