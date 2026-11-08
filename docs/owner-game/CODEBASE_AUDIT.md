# Owner Game — Evidence-Based Codebase Audit
**Audit date:** 8 November 2026  
**Repository:** `lockyeradam100/cm0102-bosman-edition`  
**Known working prototype commit:** `a06e98d38094584ac5270b329ff423c16df5d9af` on `owner-game-live-bridge`.  
**Planning branch:** `owner-game-master-blueprint`.  
**Evidence standard:** distinguish direct source review, completed build and user-confirmed gameplay; never conflate them.

## 1. Verified / implemented / unknown
| Capability | Evidence | Status and interpretation |
|---|---|---|
| Native Mac application (Intel and Apple Silicon) | Mac Actions build completed successfully for prior run; `.github/workflows/owner-game-mac-build.yml` compiles universal binary and packages ZIP | **Build verified**, but successful build alone does not prove all game loops |
| Arsenal game opens on Mac | Real screenshot confirmed game at Saturday 14 July 2001, news page | **User-verified** |
| Owner Overview | Fn + F12 pop-up displayed `Arsenal`, date `2001-07-14`, balance `GBP 27951510`, `Owner Game bridge: connected` | **User-verified read path**; simple modal, not complete UX |
| Arsenal saved game | User confirmed they started and saved Arsenal | **User-confirmed**, but compatibility with future owner DB not yet tested |
| Owner capital injection | `src/recomp_rt/win/user32.c` queues `inject_cash`; `src/recomp_rt/win/owner_game.c` applies up to £1bn to CM finance record and writes result | **Code implemented as sandbox**, not yet verified live by user; no source-of-funds/loan/equity ledger |
| Session/club guards | `owner_game.c` checks process session token, club ID, amount limit and overflow | **Present**, but not save-bound owner ID or durable idempotency |
| Match delegation | `src/recomp_rt/win/holiday_next.c` adds `Let Manager Handle Next Match` next to holiday controls; native holiday simulation takes over before fixture and restores settings | **Source implemented** and packaged in separate Windows proof workflow; **Mac user validation outstanding**, delegated fixture/state currently runtime-only |
| Contract negotiation intervention | `src/recomp_rt/win/owner_game_contract_negotiate.c` and `owner_game_willingness.c` have exact Figo/Aston Villa proof exceptions | **Experimental hooks**, not general feature; require isolation from production |
| Figo contract persistence | `owner_game_contract_submit_trace.c` comment documents failed native pending-offer persistence, tracing add/remove | **Known unresolved correctness defect**. No response is not a player rejection |
| Full personal owner finances | No owner identity, accounts, loan register or double-entry ledger found in Owner Game bridge source | **Not implemented** |
| Acquisition/manager governance | No persisted acquisition, board mandate or manager lifecycle domain in bridge source | **Not implemented as Owner Game domain**; native football features may exist and need parity audit |
| 1980 historical start | Bosman README advertises selectable 2001/02–2025/26; no 1980 package or rules shown | **Not implemented or verified** |
| Owner attendance and supporter memory | No persistent owner attendance/reputation domain found in Owner Game bridge | **Not implemented** |
| Unified owner-game save | Existing `owner-game-state.json`, `owner-game-result.json`, `owner-game-contract-event.json` are loose process/file outputs | **Not a career-safe or atomic owner save** |

## 2. Current technical structure
**Football kernel:** `src/recomp_rt/` reimplements the original 32-bit game's runtime; `src/cm3/` contains supporting C modules; compiled via `CMakeLists.txt`. The upstream's README identifies translation of original x86 program instructions into C, with native SDL2 and no emulator.

**Bridge:** `owner_game.c` obtains current human manager, club and finance from native memory; `write_state()` emits JSON; `process_command()` reads and deletes a one-shot text command. It directly mutates the club Finance record then rereads it. `testrun.c` invokes bridge hooks before and after the human turn.

**UI:** `user32.c` handles SDL keys and F12; owner pop-up parses JSON string fields from `owner-game-state.json`. No typed owner controller or canonical event model yet.

**Matchday:** `holiday_next.c` extends existing menu. Its file comment explicitly says the delegation mark is runtime state, **not** in saved game. The owner attendance pattern is a different feature and is not represented by this one-match holiday proof.

**Save:** upstream README states Mac saves normally live in `~/Library/Application Support/cm0102`. Existing bridge outputs use the user directory, not a save-specific container. A future owner sidecar must be mapped to a particular `.sav` and atomically checkpointed.

**Build:** `.github/workflows/owner-game-mac-build.yml` downloads game files, builds universal SDL2, builds app, creates `Owner-Game-Mac-Complete.zip`, and checks existence and bridge strings. It does **not** run an actual Arsenal save, send F12, submit owner funding, or assert a synchronized ledger.

## 3. High-risk implementation issues to resolve before feature expansion
**R1 — Accidental free money (critical).** Capital injection creates CM cash without debiting personal cash or recording debt/equity. Do not use as real finance gameplay. Replace with funded double-entry transaction and readback.

**R2 — Career identity bleed (critical).** Shared bridge file names and process session protect some replay errors, but do not form an owner save or map to a unique career. Two saves can share directory state; require game UUID, save manifest and scoped paths.

**R3 — Missing durable pending decisions (critical).** Figo's submitted contract proposal reportedly did not persist in native contract manager; hook must be traced and correctly committed at submit boundary, then validated over game ticks and reload.

**R4 — Delegation not durable (high).** One-fixture manager delegation uses volatile arrays in `holiday_next.c`; save/quit/restart before match may lose the mark and control intent. Need durable fixture decision and safe resume.

**R5 — Experimental exceptions in production code (high).** Specific staff/club IDs (Figo 1730, Aston Villa 374) and special owner wage logic appear in compiled source. Must isolate them behind test-only settings/fixtures and ensure normal players receive native rules by default.

**R6 — GUI command is not transactional (high).** Owner investment command is a file write followed by action at a later human turn; UI reports 'queued' but no durable acknowledgement, economic validation or exact-once guarantee. Replace with command journal and lifecycle.

**R7 — Ownership absent (high).** Current human manager pointer is not a legally defined owner. Owner, controller and football manager must have separate stable IDs and authority.

**R8 — Incomplete tests (high).** Build passes are not feature acceptance. Add automated and human-run game save probes, event tracing, crash/resume and parity comparisons.

**R9 — Historical data (high).** Present code/package supports 2001 onwards; a genuine 1980 scenario needs historical data licensing/provenance and rules research, not a date transform.

**R10 — Rights/distribution (release blocking).** The repository identifies GPLv3 for project-authored code only, not the original game/assets or trademark. Its README says the unofficial port is unendorsed; official game files and third-party season databases have separate rights. Before distributing/selling Owner Game, commission qualified rights/legal review of copyright, trademarks, databases, derivative components, GPL compliance and permissions. This audit does not assert commercial distribution is legally cleared.

## 4. Technical P0 / P1 tests to implement
| Test ID | Fixture | Expected |
|---|---|---|
| MAC-BOOT | App from delivered ZIP | Native universal app launches and loads a new Arsenal 2001 career |
| MAC-OVERVIEW | Arsenal 14 Jul 2001 | F12 identifies correct club/date/balance; no game input corruption |
| FIN-PROTOTYPE | Arsenal sandbox, test-only £5m | CM delta exactly £5m at safe boundary, no repeated command; label as unsupported final economics |
| FIN-FUNDED | Owner has £100m; funds £5m loan/equity | Owner wallet −£5m, club cash +£5m, correct debt/equity and ledger, persists |
| FIN-REJECT | Owner cash £2m; tries £5m | Zero effect with explicit insufficient cash rejection |
| FIN-DUP | Submit same idempotency key twice | Exactly one £5m transaction |
| FIN-CROSSSAVE | Switch Arsenal career to different club save | No stale owner decision or bridge file can act on new career |
| CONTRACT-PENDING | Send player/staff offer then save before response | Offer remains pending or resolves with recorded response; no silent loss |
| CONTRACT-FIGO | Figo → Aston Villa repro, record add/remove/resolve path | No false accepted/rejected attribution; native pending offer real or explicitly fail |
| DELEGATE-MATCH | Arm next first-team fixture | CM runs match without user watching; result/news/standings and control restored |
| DELEGATE-RESTART | Save after arming but before fixture | Delegation survives restart, no duplicate result |
| ATTENDANCE | Owner attends home and away, skips another | Persistent profile/events independent of manager's football handling |
| OWNER-MANAGER | Appoint/replace manager, delegate match | Correct native staff role/compensation, permissions and performance continuity |
| SAVE-ATOMIC | Interrupt during save | Last complete paired owner + football snapshot loads |
| FOOTBALL-SEASON | Full season and rollover | Every core operation exists or delegates; no broken league/player/contract rules |
| HIST-1980 | Launch 1980/81 scenario | Correct-age data, dated rules/economics, factual provenance |
| COMPAT-UPGRADE | Load Arsenal old save after new app | No data loss; owner-domain migration adds only explicit new fields |

These are a **test plan**, not claims that all tests have passed.

## 5. Source and evidence index
- `README.md`: platform, season support, saved-game paths, rights caveats and GNU GPLv3 code licence.
- `CMakeLists.txt`: targets/runtime and tests.
- `src/recomp_rt/win/owner_game.c`: club state, one-shot capital, contract resolution observation.
- `src/recomp_rt/win/user32.c`: Owner Overview/F12 and UI command generation.
- `src/recomp_rt/win/testrun.c`: human turn hook.
- `src/recomp_rt/win/holiday_next.c`: native one-match delegation prototype.
- `src/recomp_rt/win/owner_game_contract_negotiate.c`, `owner_game_contract_submit_trace.c`, `owner_game_willingness.c`: Figo-specific test hooks and persistence trace.
- `.github/workflows/owner-game-mac-build.yml`, `owner-game-matchday-build.yml`: build proof scope and limitations.
- Mac test screenshot confirmed 8 Nov 2026: Arsenal, 14 Jul 2001, owner bridge connected with £27,951,510.

## 6. Recommended technical verdict
**Keep the Bosman football kernel for now**. We have a proven native Mac program, a rich existing simulation, historical 2001–2025 season framework, and a real state integration point. Do not immediately throw that away and build a new match engine.

**Do not keep stacking live JSON controls on the bridge.** Extract proper ownership/world/ledger domains, save linkage and a typed interface. The Bosman adapter is justified only as long as it can pass safe native action + save/reload proofs for contracts, management and matchdays. Schedule an explicit re-evaluation after P1/P2 if those proofs fail; then assess a separate native game core or replacement football kernel with cost/rights implications.

**Critical distinction:** this audit is a product/architecture assessment from current source and one successful user-observed Mac integration, **not** a broad end-to-end gameplay certification.
