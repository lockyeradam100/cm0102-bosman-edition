# Owner Game — Decisions, Assumptions and Open Risks
**As at 8 November 2026.** Decisions below codify previous Owner Game discussions and minimise repeated interruptions. They may be revisited by an explicit change of product vision or failed feasibility proof, not because a single implementation is inconvenient.

## Product decisions already established
| ID | Decision | Basis / consequence |
|---|---|---|
| D-01 | **Owner-first football/business sandbox**, not a reskinned manager simulation | Strategic ownership decisions matter, full football world underneath |
| D-02 | **CM01/02 level of complete football functionality is a release gate** | No expanding optional business layers while essential football actions are incomplete |
| D-03 | **Classic, fast, readable UI; deep simulation below** | No unnecessary FM26-style mandatory chores |
| D-04 | **Historical signature starts around 1980** | 1980/81 is default target for research; 2001 is engineering test base, never a substituted historic release |
| D-05 | **Owner hires, supports, challenges, poaches and fires manager** | Manager has agency and authority, compensation and consequences |
| D-06 | **Optional owner-manager authority** | Owner can take charge of team/tactics but is not forced to |
| D-07 | **Attend home/away, irregularly or never; owner can skip matches** | CM simulation generates instant result; owner visibility and relationships persist |
| D-08 | **Owner wealth separate from club money** | Capital injections funded by bank/asset/credit and have debt/equity accounting |
| D-09 | **Player and manager career agency, delayed negotiations, honest pending states** | No silent rejection (Figo), realistic ageing/ambition/wage expectation, manager tapping-up risk |
| D-10 | **Real academy pathways; background youth/reserves and foreign leagues** | Strong career-world realism without forced youth match control |
| D-11 | **Delegated repetitive work** | Training, press, staff renewals delegated by default where permitted; Manual/Advise/Delegated/Autonomous guardrails |
| D-12 | **Football parity then intelligent enhancements** | Better world memory, career choices, recruitment and AI than CM, without reinventing match simulation prematurely |
| D-13 | **Preserve live Mac prototype as evidence, not architecture** | Build the common contracts and automated tests on a separate planning branch |
| D-14 | **Build large integrated systems in controlled stages** | Every stage needs a complete game journey and save/reload checks |

## Technical defaults selected now (no user response needed)
| ID | Current default | Alternative only if evidence forces review |
|---|---|---|
| T-01 | Keep Bosman as authoritative football kernel | Replace/decouple only if tested adapters cannot reliably mutate and persist native actions, or licensing blocks the planned product |
| T-02 | Modular monolith with typed adapter and durable owner world | Separate process later, if time/segregation/performance requirements justify it |
| T-03 | SQLite-style ACID embedded store for owner ledger and events | Versioned binary file if SQLite integration proves incompatible, retaining same domain contracts |
| T-04 | A single Continue clock shared by football and business | No independent real-time business engine during football progression |
| T-05 | First finance proof: **funded, interest-free shareholder loan** | Offer equity and interest-bearing loan as genuine alternate finance instruments |
| T-06 | Base test career is Arsenal July 2001; baseline balance £27,951,510 from observed screenshot | Values change per save; never hardcode Arsenal into generic rules |
| T-07 | Default owner-only play with manager delegated match operations | Allow owner-manager toggle; never require watching matches |
| T-08 | Mac packaged-app smoke is mandatory for every milestone | Windows and Linux targets follow, not substitutes for Mac test |
| T-09 | Development world first in validated supported 2001 season, launch-historical target 1980/81 | A different early 1980s start only after historical data feasibility assessment |
| T-10 | Single-club controlling owner experience first; model shareholdings for future multiple-club operation | Don't delete portfolio-aware data model to simplify first UI |
| T-11 | Event log/decision history and exact-once transactions across saves | No UI-only state as substitute |
| T-12 | Native CM retains football decisions and gives meaningful response; ownership authority adjusts mandate not acceptance | Exception and influence rules explicit/tested, never blanket overrides |

## Questions only evidence can answer
- **F-01:** Can we safely hook native contract submission so a genuine offer remains pending after time advances and reload?
- **F-02:** Does the one-match delegate path safely resume after a save, reload, unexpected holiday setting, or roster/manager switch?
- **F-03:** Can football club finances be observed sufficiently to populate a trustworthy cash flow and P&L, instead of only an opening cash balance?
- **F-04:** How is game save state captured without partially committing independent owner/football states?
- **F-05:** Can staff changes, appointment, dismissal and compensation be fully wired to native CM structures?
- **F-06:** What historical 1980 databases, club/competition rules, financial values and provenance are available for defensible creation?
- **F-07:** What intellectual property permissions, trademarks, database rights and GPLv3 distribution obligations apply before release/sale?

These are **engineering, research or professional legal gates**. Do not ask the user to guess their answers.

## Genuine player-experience choices deferred until prototypes justify them
- Exact default starting cash distribution by scenario and owner wealth origin (e.g. ordinary entrepreneur, successful investor, consortium, ultra-wealthy benefactor). Build a scenario editor; no arbitrary single default £100m for every owner.
- Depth and frequency of optional commercial/property mini-world play, informed by a functioning football + owner career first.
- Visual identity beyond core CM clarity, with playable feedback rather than endless theoretical theming.
- Commercial release, pricing or monetisation plan; rights clearance must precede a distribution plan.
- Historical start menu breadth beyond the mandatory ~1980 signature.

## Policy for future changes
A proposed feature is accepted only if it:
1. Fits an existing domain or explicitly changes the master architecture with dependencies accounted for.
2. Defines entity → action → consequence → history → persistence.
3. Respects finance, role and football parity invariants.
4. Comes with an integrated playable acceptance scenario and a rollback plan.
5. Does not delay the P1 football-completeness gate for speculative optional complexity.

**Current user input required: none.** The programme has enough design direction to proceed through P0, P1 audits and P2 technical proof work. Matters requiring external legal rights cannot be decided by implementation alone.
