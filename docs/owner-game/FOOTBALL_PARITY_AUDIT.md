# Football Completeness Audit — CM01/02 Owner Game Gate
**Version:** initial evidence register, 8 November 2026.  
**Important:** This is a **test matrix, not a pass certificate**. `BASE` means the Bosman/CM kernel demonstrably has a relevant foundation, **not** that the entire Owner Game flow is verified. `UNTESTED` means code inspection/user playthrough insufficient to certify. `DEFECT` is a known exception. Only `PASS` with evidence may close a row.

Required fields for each future audit update: **source location/feature, interactive path, delegation path, AI path, save state, test command or gameplay evidence, release status.**

| ID | Minimum professional football operation | Evidence at baseline | Open validation | Gate |
|---|---|---|---|---|
| F01 | Clubs, names, divisions, squads and ages | BASE — packaged CM / season database | 1980 data separate | UNTESTED |
| F02 | Manager/team-controlled playable career | BASE — Arsenal save launches | Multi-season continuity | UNTESTED |
| F03 | Team selection, injured player handling, bench | BASE — CM football system | Full selection and restrictions | UNTESTED |
| F04 | Realistic formations and positional roles | BASE — CM tactics; research inventory | Player and delegated AI behaviour | UNTESTED |
| F05 | Captaincy and set-piece assignments | CM-class baseline anticipated | Persist changing choices | UNTESTED |
| F06 | Match tactical changes / substitutions | CM-class baseline anticipated | Home/away conditions, role control | UNTESTED |
| F07 | Match engine score, cards, injuries, fitness | BASE — native CM engine | Deterministic regression & aftermath | UNTESTED |
| F08 | Instant match results when owner skips | `holiday_next.c` delegation prototype | Real Mac result, return of control | UNTESTED |
| F09 | One-fixture delegation persistence | In-memory arrays in `holiday_next.c` | Save/reload mid-delegation | DEFECT |
| F10 | Domestic leagues, promotion/relegation | BASE — season engine | Whole season in selected years | UNTESTED |
| F11 | Domestic cups and cup replays/rounds | BASE — coded season rules | Complete rounds/season | UNTESTED |
| F12 | Continental competitions and qualification | BASE — extensive rules research | Multiple seasons incl restructuring | UNTESTED |
| F13 | International football/qualifications | BASE — coded calendar / competitions | International calendar/club conflicts | UNTESTED |
| F14 | Foreign / background league progression | BASE — coded league histories | Credible multi-year active world | UNTESTED |
| F15 | Friendlies and pre-season | CM-class baseline anticipated | Fixtures, match reports, injuries | UNTESTED |
| F16 | Fixtures, live tables, scores, league history | BASE — native competition system | Club world and cross-league updates | UNTESTED |
| F17 | Referee/weather contextual information | CM-class baseline anticipated | Affect engine appropriately if supported | UNTESTED |
| F18 | News/media and manager-board pressure | BASE — CM native news and board; screenshot | Multi-season pressure, reputational AI | UNTESTED |
| F19 | Player contracts: variation, expiry, bonuses | BASE — native contract manager | Terms correct across saves and time | UNTESTED |
| F20 | New contract offer pending until actual outcome | Figo→Aston Villa trace says submit not persisted | Submit→pending→reply/reload | DEFECT |
| F21 | Player renegotiation, counter-offers & expiry | CM-class baseline anticipated | Respond after delay, no silent loss | UNTESTED |
| F22 | Player wages and career/club ambition | CM native behaviour partially observable | Believable stage/age/performance decisions | UNTESTED |
| F23 | Transfer bids/acceptance/agents/instalments | CM-class baseline anticipated | Financial and squad consistency | UNTESTED |
| F24 | Loans and recalls / club permission | CM-class baseline anticipated | Start/end and squad/registration | UNTESTED |
| F25 | Free-agent behaviour and ability to sign | CM-class baseline anticipated | Realistic salary/role demands | UNTESTED |
| F26 | Releases and mutual termination | CM-class baseline anticipated | Severance, registration, budget | UNTESTED |
| F27 | Manager/staff search and recruitment | CM-class baseline anticipated | Staff appointment, compensation | UNTESTED |
| F28 | Hiring contracted managers / club buyout | Owner Game requirement | Permission, informal talks, payments | UNTESTED |
| F29 | Manager support/review/sacking | Native board/manager partial | Owner authority, successor, severance | UNTESTED |
| F30 | Training and autonomous delegation | CM native training anticipated | AI delegation + injury/time impacts | UNTESTED |
| F31 | Scouting and realistic lower-division recruitment | CM-class baseline anticipated | Geographic/league/financial reality | UNTESTED |
| F32 | Registration, work permits, foreign quotas | BASE — season-specific rules | Cross-era validity and AI compliance | UNTESTED |
| F33 | Injury recovery, suspensions, appeal logic | BASE — native CM | Correct rules/disciplinary timings | UNTESTED |
| F34 | Reserve/youth competition sim in background | Owner Game requirement | No forced reserve manual play | UNTESTED |
| F35 | Academy intake & viable first-team pathway | Owner Game requirement | 10-season longitudinal player outcomes | UNTESTED |
| F36 | AI clubs buy/sell, recruit/fire, develop | BASE — native CM AI | Economic constraints + credible world | UNTESTED |
| F37 | Player promises, unhappiness and morale | CM native foundations anticipated | Persistence + consequences | UNTESTED |
| F38 | Manager/player personalities and relationships | CM native foundations anticipated | Owner Game added memory, risk | UNTESTED |
| F39 | Save and resume football world | User saved Arsenal | Real owner+football save upgrade | UNTESTED |
| F40 | Multiple season rollover with rules change | Bosman advertises 2001–2025 rules evolution | 3/10/15 season test replay | UNTESTED |
| F41 | Full human football-management mode | CM-based native kernel | Entire season with no owner overlay | UNTESTED |
| F42 | Explicit owner vs manager control switch | Owner Game requirement | Authority + squad/fixture transfer | UNTESTED |
| F43 | Owner attendance vs abstention | Owner Game requirement | Fan memory separate from match engine | UNTESTED |
| F44 | Football and business decisions share one date | Current bridge human turn only | Cross-domain same-day invariants | UNTESTED |

## Test matrix dimensions and evidence standard
Every row is checked through **seven perspectives**: (1) visible information, (2) human operation, (3) delegate/AI pathway, (4) valid and invalid transitions, (5) football result/career consequence, (6) save and reload, (7) 1–10-season repeatability. An operation is not `PASS` if it only has a screen or hidden native function.

For changed football behaviour, compare a seeded save with unmodified Bosman:
- Verify fixture results and state deltas with no owner intervention.
- For intentional owner overrides, assert only the specified affected records differ (e.g. transfer authority limit, not player willingness).
- Record the event chain and update the case from `UNTESTED`/`DEFECT` to `PASS` only after testing.

## First regression suites
**Suite A — Arsenal 2001 base:** load, save, team tactics, transfers, staff search, contracts, friendlies, fixtures, a complete season and rollover.  
**Suite B — Figo Aston Villa defect:** submitted native offer, contract manager add/remove traces, pending/readback, calendar advancement, reload, acceptance/rejection with explicit reason.  
**Suite C — Delegation:** one match under manager, cancel before match, save/reload while armed, schedule conflict, holiday settings restored.  
**Suite D — League / world:** England + European and foreign-background leagues, cup entry, international duty, season rollover, players/staff aging, economic/contract continuity.  
**Suite E — Long-run:** 3/10/15 seasons and successful save migration under repeated game version upgrades.

## Audit gate
**P1 fails while F09 and F20 remain defects, or any required F01–F44 operation is untested/missing.** Product work on detailed external business worlds should not proceed in parallel with untreated football defects. Infrastructure for IDs, journaling and save reliability may be developed to unblock this gate.
