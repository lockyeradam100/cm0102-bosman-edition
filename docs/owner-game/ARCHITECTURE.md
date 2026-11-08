# Owner Game — System Architecture, Records and Integration Contracts
**Decision level:** proposed target architecture; not represented as already implemented.  
**Starting baseline:** a native Mac/Windows `cm0102_recomp` executable and a file-based C bridge with live Arsenal state readback. Source pointers are in [CODEBASE_AUDIT.md](CODEBASE_AUDIT.md).

## 1. Architectural decision
Adopt a **modular monolith with a clear football-kernel adapter and an owner-domain layer** for the first integrated release. Do **not** build a collection of unrelated overlays, independent clocks or manually updated JSON files. This preserves the native Bosman match/football kernel while letting the owner systems mature independently. An external process is not required in the first version; a separate process/sidecar may be introduced *after* validating safe clock, save and event semantics.

```text
                    OWNER GAME NATIVE APPLICATION (Mac first)
  +---------------------------------------------------------------+
  | CM-style navigation, inbox, views, decision panels, reports  |
  +-----------------------------+---------------------------------+
                                |
                          Game application API
                                |
  +-----------------------------v---------------------------------+
  | World / Time / Decision / Authority / Notification / History  |
  | Shared IDs | event dispatch | state projection | permissions  |
  +------------+-----------------------+--------------------------+
               |                       |
  +------------v------------+ +--------v-------------------------+
  | Ownership/business      | | Football adapter (Bosman)         |
  | Owner wallet & holdings | | CM contract/manager/club/player   |
  | General ledger          | | Competition, match, saves         |
  | Board/governance        | | Finance cash source of truth      |
  | Commercial/property     | | Sanctioned reads/writes/events    |
  +------------+------------+ +------------------+---------------+
               |                                 |
               +---------------+-----------------+
                               |
                    Atomic, versioned game save
                      + migration / replay tests
```

**Ownership of truth**
- Bosman/CM remains authoritative for native football state: teams, squads, injuries, staff/players, contracts, fixtures, results, tactical outcomes, competition standings and its existing core finance balance until an engineered handover proves safe.
- Owner domain remains authoritative for owner identity, holdings, board mandates, approval limits, promises, personal money, related-party debt/equity, the ledger and owner-world relationships.
- The **finance integration boundary** is a controlled synchronisation contract. The club has only **one canonical cash balance**. When a valid owner funding transaction is committed, it is posted to Owner Game ledger and applied to CM's club balance once; the system then reads CM to confirm exact agreement. No competing cash totals that silently overwrite one another.
- Time and event ordering are centralised: one authoritative in-game date/event sequence. No sidecar advancing a different date and guessing when CM has processed a match.
- Historic rules and economics are versioned `SeasonRules`; contracts refer to the rules that applied when agreed.

## 2. Canonical IDs and data model
Do not use pointers, names or transient game-session slots as permanent identifiers. Persist stable IDs and map to the recompiled game's club/staff IDs through an adapter:
```text
GameWorld(game_id, schema_version, season, current_date, seed, step_seq)
Owner(owner_id, identity, profile, reputation, visibility, wealth_policy)
OwnerAccount(account_id, owner_id, currency, type, balance)
ClubRef(world_club_id, cm_club_id, name_history, country)
Holding(holding_id, owner_id/holding_company_id, club_id, shares, votes,
        acquisition_cost, valuation_policy, effective_dates)
BoardMandate(mandate_id, club_id, manager_id, authority_limits, objectives,
             guardrails, review_dates, status)
Appointment(appointment_id, club_id, person_id, role, contract_id, hired_date, exit_date)
Contract(contract_id, parties, terms, effective_date, expiry, version, status)
Offer(offer_id, negotiated_object, counterparty_ids, terms, sent_date,
      response_due, status, status_reason, history_event_ids)
Promise(promise_id, from_id, to_id, terms, due_date, status, consequences)
Event(event_id, game_id, seq, at_game_date, type, actor_id, subject_ids,
      payload, authority_decision_id, provenance, idempotency_key)
Decision(decision_id, event_id, actor_id, mandate, options, expires_at,
         resolution, consequence_ids)
FinanceTransaction(txn_id, type, at_game_date, status, approved_by,
                   currency, amount, related_entity_ids, source, idempotency_key)
LedgerEntry(entry_id, txn_id, entity_id, account_id, debit, credit, currency)
ClubFinanceSnapshot(snapshot_id, club_id, date, cm_cash, ledger_cash,
                    reconciled, projection_hash)
MatchEngagement(match_id, owner_id, attendance_mode, decision_time, actual_presence,
                supporter_visibility_event_id)
Property/Facility/CommercialAgreement/BusinessPortfolio (separate bounded aggregates)
```
Player and club names vary over decades, so `world_club_id` and `person_id` are never derived from names. An incoming CM ID is validated against the save's game/world identity and mapping version; stale commands cannot target a different club after a new save is opened.

## 3. Ledger contract (mandatory for all financial features)
Use **double-entry postings** by legal entity with balance to zero per transaction:
- Owner pays £5m equity: owner `Dr investment_in_club / Cr bank 5m`; club `Dr bank / Cr paid_in_capital 5m`.
- Owner pays £5m interest-free shareholder loan: owner `Dr loan_receivable / Cr bank 5m`; club `Dr bank / Cr shareholder_loan_payable 5m`.
- Interest-bearing loan creates principal as above and accrues `Dr club_interest_expense / Cr interest_payable` and owner `Dr interest_receivable / Cr interest_income` on the contractual schedule.
- Club repays principal: `Dr club_shareholder_loan_payable / Cr club_bank`; owner `Dr owner_bank / Cr loan_receivable`.
- Equity has **no guaranteed contractual repayment**. A dividend is a distinct restricted transaction, not a loan repayment. Valuation changes affect net worth, not automatically bank cash.
- Owner can fund via the personal bank, a holding company, co-investor equity or external borrowing; each route requires funded postings and separately attributed obligations.

**Invariant FIN-01:** for every legal entity and currency, sum(debits) == sum(credits) per posted transaction.
**FIN-02:** the owner/holding-company cash must not go negative unless an expressly modelled overdraft/borrowing facility permits it.
**FIN-03:** every club cash-changing owner transaction has a corresponding verified Bosman cash delta exactly once.
**FIN-04:** no owner-loan repayment is performed without an outstanding principal and a valid payment.
**FIN-05:** investments and loans are separate instruments; display owner cash, investment carrying value, loan receivables and total net worth separately.
**FIN-06:** club losses, transfers, wages, sponsorships and ticketing have exactly one posting integration route; avoid a second ledger counting the same CM event.
**FIN-07:** trial balance and account-to-CM reconciliation reconcile before save/checkpoint.

Initially model a **control/subledger**, not a pretend comprehensive accounting integration. Capture opening CM bank position and use traceable incremental postings. Do not claim the whole P&L is accurate until source native CM income/expense events or reliable reconciliation are captured. Build native P&L adapters, then switch audited scopes to fully ledger-authored records.

## 4. Command, event and decision protocol
A single authoritative command bus processes player and AI requests at a safe boundary. Each command carries:
```json
{
  "schema_version": 1,
  "game_id": "game-uuid",
  "command_id": "stable-uuid",
  "issued_game_seq": 1234,
  "actor_id": "owner-001",
  "subject_id": "club-arsenal",
  "action": "owner.invest",
  "payload": {"amount_minor": 500000000, "currency": "GBP", "method": "shareholder_loan_interest_free"},
  "expected_version": 17
}
```
- Validate game identity, session, club, authority, available funds, legal/era rules, credit, idempotency, transaction preconditions and relevant version.
- Return **rejected with reason** OR create one durable pending decision/transaction.
- Run through transition `proposed → approved → reserved → applied → reconciled → committed` or `rejected/rolled_back`; do not tell a player an investment succeeded when only queued.
- Command retries use `command_id` and `expected_version`; duplicate submissions return the previous outcome, never post again.
- Journal decisions and engine effects: `CommandAccepted`, `LedgerPosted`, `CMEffectApplied`, `CashReconciled`, `DecisionCompleted` with causal IDs. Events carry human-readable reason and what changed.
- Side effects in the CM engine must run at deterministic, tested safe points—not from SDL's UI callback or while the engine is mid-save.
- Create auditable compensating transactions when true rollback is impossible; do not silently rewrite historical ledger entries.

**Contract negotiation lifecycle:** `draft → sent → acknowledged/pending → counteroffer/accepted/rejected/expired/withdrawn`. Pending is an explicit durable state; no response is *not* rejection. In the Figo regression case, the player's offer remains pending until CM delivers an actual response or a valid timeout; test this across ticks and reload.

**Manager lifecycle:** `candidate → approached → interviewed/tapped_up → club_permission/buyout → negotiation → agreed → appointed → active → under_review → resigned/dismissed/expired`; authority charter, severance, club compensation and time delay are recorded.

**Matchday lifecycle:** `fixture_scheduled → attendance_choice → delegated_or_managed → played_by_CM → result_committed → match_report → reputation/financial consequences`. Non-attendance is an explicit valid path; at no point does the owner have to sit through the match simulation.

## 5. UI contract and game-time loop
```text
Open save → load Bosman save + Owner Game save → compare IDs/hashes →
reconcile projections → present Owner Home
    |
    v
Human action or delegation → validate commands → queue decisions
    |
    v
Continue → apply approved pre-turn effects at safe point →
advance native football simulation exactly once →
capture match, contracts and financial events →
run owner/business and AI decisions for that same day →
reconcile and commit event journal → display decisions/reports
    |
    v
Save → atomic pair checkpoint of football + ownership state
```
No parallel unmanaged timer; one Continue event must never resolve two identical matches or double-apply an investment.

The UI talks to a typed `GameApplication` interface: `getOwnerHome`, `getClubFinance`, `getPendingDecisions`, `issueCommand`, `getManagerBoard`, `getFixtureEngagement`, `getHistory`, `saveGame`. No screen parses changing JSON string literals as its business model. The early F12 overlay may temporarily call a compatibility adapter but must be retired once native pages exist.

## 6. Save and migration (blocking requirement)
**Current baseline:** Bosman `.sav` files are in `~/Library/Application Support/cm0102` on Mac; the bridge writes nearby text/JSON files. Such shared sidecars are NOT save-bound, transactional or automatically proven persistent across careers. This is a major gap.

**Target:** Each career has a versioned manifest and atomic checkpoint:
```text
career_<id>/
  manifest.json                 # game_id, last_seq, schema, football hash, owner hash
  football.sav                  # CM save snapshot
  owner.db                      # durable owner-world and ledger store
  journal/                      # command+event sequence and idempotency records
  checkpoints/                  # last-known-good generations, atomic manifest switch
```
A local embedded DB (SQLite preferred for ACID transactions and indexes) is a proposal, not a dependency currently in the build. Single-file portability is allowed only after a validated save contract exists.

**SAVE-01:** save and reload keep owner cash, club cash, loans, holdings, mandate, negotiations, fixture decisions, attendance, reputations, promises and pending timeouts.  
**SAVE-02:** a crash or forced quit mid-commit reloads last **fully consistent** generation, never a half-updated club/owner pair.  
**SAVE-03:** opening a different save cannot execute old commands or read another career's owner identity.  
**SAVE-04:** migrations are forward-tested and keep backup of old save; a new executable can load previous known-good career.  
**SAVE-05:** results and financial transactions are identical before/after 1, 10 and 100 save/reload iterations.  
**SAVE-06:** every nightly/integration build has at least one real Mac saved-career smoke test, not just compile-and-package success.  
**SAVE-07:** no losing the football game's standard save without warnings/backup on upgrade.

Atomicity between native `.sav` and the owner DB requires checkpoint generation and validation, **not** two independent filesystem writes called 'atomic'. Use staged generation files + checksum and a final manifest pointer; on failure discard unreferenced staged generation.

## 7. Football adapter and performance
- Keep `CMFootballAdapter` deliberately small: safe read snapshots, stable ID mapping, approved finance modifications, manager/contract authority hooks, match scheduling + resolution boundary, native save/load hooks, and observable transaction results.
- Every changed native decision path is exercised against unmodified CM with fixed seed and the same save. Explicit deviations require business authority rationale and a regression test.
- Native contract hooks currently contain staff/club-specific test logic. Move experiments behind test-only fixtures before release; never hardcode one famous player's outcome in the shipping engine.
- The football engine can simulate many days; owner layer should subscribe to **events and changed snapshots** rather than scan entire world every tick.
- Simulated AI rivals use the same core command/approval/ledger and contract state machines as the human, though narrative processing may use cheaper approximations.
- Benchmarks: cold load, active day tick, mass fixture days, 10 seasons, save/load; no fixed performance target is asserted before measuring the current baseline.

## 8. Historical data and rules
`HistoricalScenario(year, region, club_set, squad_database, finance_baseline, rules_version, economic_parameters, source_provenance)`.
- First verify the supported base 2001–2025 seasons; do not present them as 1980.
- A true circa-1980 start requires age-accurate players, club status, competitions, league sizes, results/history, stadiums, wages and ownership economics. Provenance must be recorded; uncertainty is explicit.
- `SeasonRules` changes by effective date; newly enacted rules affect future events/contracts appropriately, not retrospectively and arbitrarily.
- Currency denomination, money inflation and historic revenue scales use scenario-specific models; a 1980 £5m decision must not be treated like a 2026 £5m decision.
- When real history diverges, game-generated alternative outcomes become authoritative for that career, rather than overwriting them with canonical future results.

## 9. Integration testing rules
**Six-stage bridge proof:** READ → IDENTIFY → INFLUENCE → CONTINUE → READBACK → PERSIST.

A new feature is not 'done' until:
1. Same club/person and game/season identity in both systems.
2. A decision has a visible reason, authority, and valid counterparty.
3. It changes engine state **only** at a safe point.
4. After advancing the game, consequences match expected game model.
5. A save/reload retains both sides and the event history.
6. AI or player can proceed through a complete coherent lifecycle, and a negative/malformed action is safely rejected.
7. Regression test verifies original CM football behaviour unless a deliberate override is specified.
8. Tested on a packaged Mac app, not only a developer build.

**Never merge**: arbitrary balance patches, owner-specific club cheats, a F12 prototype mistaken for full UI, non-persistent promises/offers, or an unverified build presented as playable.

## 10. Refactor sequence
1. Preserve a tagged/branch-protected known-good Mac prototype and Arsenal save; identify all existing native bridges and hardcoded experiments.
2. Build game-ID and versioned ownership store with a no-op readback scenario.
3. Add durable event/command journal and idempotent processing; pass save/reload and crash tests.
4. Implement double-entry owner/clubs ledger and **one** integrated owner-funded transaction with exact readback and persistence (no free cash).
5. Implement actor/authority/decision state machine shared by manager hiring, transfer approval, and boardroom.
6. Integrate match delegation/attendance as one fixture lifecycle, while preserving CM outcome simulation.
7. Replace modal F12 with Owner Home and context pages, maintaining keyboard control and Mac usability.
8. Expand from validated 2001 base to researched 1980 scenario with historical rules.
9. Add commercial, facilities and optional business modules only onto the verified shared model.

This is a **proposed architectural blueprint**, and each important Bosman interaction requires a proof of feasibility before implementation can be declared complete.
