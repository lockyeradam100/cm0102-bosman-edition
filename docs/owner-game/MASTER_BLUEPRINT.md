# OWNER GAME — Master Game Design Blueprint
**Status:** Foundational product specification, 8 November 2026  
**Motto:** DEEP SIMULATION. SIMPLE CONTROL. OWNER FIRST.  
**Baseline:** Working Mac integration at `a06e98d38094584ac5270b329ff423c16df5d9af`; this is a technical prototype, not a complete feature.

## 1. The product
Owner Game is a persistent, historically evolving football ownership and business sandbox with a fully credible Championship Manager 01/02-quality football game underneath. The player may be an owner/chairman, an owner who directly manages the first team, or (where the selected mode permits) a traditional manager. The primary fantasy is **building, governing, financing and ultimately shaping a football institution**—not merely editing the club balance.

Signature experience: start around **1980**, acquire or control a football club, employ and support a manager, fund or discipline a business, develop talent, expand the ground, navigate the changing commercial football landscape, and live with consequences over decades. The world reacts and remembers. Genuine alternative history emerges from decisions.

**Non-negotiables:**
- Football is fully functional first: every real professional-club football operation is **playable or demonstrably delegable**. A missing football feature is not excused because the ownership layer exists.
- Simple CM01/02-style navigation and quick decision-making on the surface; interconnected, deep underlying systems. Avoid a cumbersome FM26-like chore simulator.
- Real connected accounts and ownership; there is no money creation without a source. Cash, wealth, loans, equity, liabilities and transfers reconcile.
- All consequential decisions have an owner, authority, state, clock, action, consequence and history; persistence across save, close, restart and years is mandatory.
- The manager owns football operations unless the player deliberately takes those powers. A club owner must **never have to watch/play matches** to advance.
- No deterministic positive boost for attending games or spending money; context, opportunity cost, law and personalities matter.
- Preserve the Bosman/CM football game as the reference for the football experience, then exceed it in player/manager agency, world memory, nuanced negotiations, ownership and youth development.
- Do not overbuild eccentric tactic/formation editors; strong real formations, sensible roles and set pieces are enough.
- Local-first single-player Mac and Windows usability and resilient saves; Windows/Linux are compatible target platforms rather than reasons to delay Mac quality.
- Business/property/mini-world expansions must serve the owner's football story and not bury the core game.

## 2. Game roles and authority
| Role | Player controls | Delegated by default | Non-negotiable behaviour |
|---|---|---|---|
| Owner/chairman | Acquisition, strategy, board, manager, funding, major budgets, assets, commercial direction | Line-ups, match tactics, training, day-to-day football operations | Appoints, supports, challenges, sacks and poaches managers; manager runs matches |
| Owner + manager | All owner decisions plus football tactics, recruitment, selection and games | Press, training, staff contract renewals and routine tasks (configurable) | Switching control never creates another owner or rewrites a contract |
| Manager-only (compatibility/optional) | Complete CM-level football management | Owner controls corporate investment/board | Must remain an enjoyable standalone football-management game |

**Authority charter** for each club defines who may approve: wages, renewals, releases, transfer bids, loans, budgets, compensation, captains/set-pieces, match lineup and tactical interventions. Owner exceptions can overrule financial limits but cannot automatically make a player accept a deal. A manager can be unhappy, resign or refuse to cooperate. Each approval writes one decision event; promises and broken promises persist.

Delegation modes for each operational function: **Manual / Advise / Delegated / Autonomous within guardrails**. Display exactly what the delegate will do and when escalation is required. Discretion and responsibility differ by role, trust, contract and owner style.

## 3. Primary game loops
**Acquire → Govern → Observe → Decide → Delegate → Advance → Respond → Review → Reinvest / Exit.**
- Start with an owner identity, starting liquidity and portfolio (or a configured inherited/consortium ownership scenario), select a historical season and buy or take control of a club.
- Create an owner strategy and 1–5 year plan: financial posture, playing philosophy, manager autonomy, academy, stadium, commercial growth and risk appetite.
- Hire the manager, coach structure and executive team; delegate ordinary work; make exceptions and board decisions.
- The simulation advances at the player's pace, surfacing meaningful decision points. A week with no exceptional business should advance rapidly. Matchday can be instant, attended as an owner, or managed tactically.
- Review finances, results, performance, supporter mood and commitments; intervene with consequences; transfer/renew/sell capital and assets.
- Rival owners, clubs, managers, agents, players, communities, regulators and sponsors also act while the player's club advances.
- Careers persist after resignations, relegation, takeovers, bankruptcy, changing clubs and historical decades.

## 4. Football minimum-complete contract
This is an **absolute gate**, not a collection of stretch features.

**Clubs and world:** leagues, cups, Europe, international games, promotions/relegations, foreign/background leagues, standings and live scores, credible scheduling, fixtures, preseason and friendlies, reserve/youth teams simulating in the background, referee/weather, competitions/history and season rollover.

**People and employment:** varied player/staff histories, age development, injuries, fitness, suspensions, work permits, registration, values, contracts/expiry, releases and mutual termination, free-agent decisions, loans, transfer bids, fee instalments, agent/club interest and division-level recruitment. Manager and staff search, approach, hire and dismissal work without dummy placeholders.

**Negotiation intelligence:** offers remain pending until a response, expiry, rejection, withdrawal or accepted resolution is recorded. Players consider role, salary, ambition, existing deal, form, age, club prestige, playing time and promises. Older players may accept shorter/lower deals; strong performers may request better terms or a move. Poaching contracted managers requires approach, discretion, buyout/compensation and reputational/legal consequences; taps-ups and informal talks must have meaningful risk.

**Football decisions:** meaningful formation/roles, team selection, captaincy, set pieces, coaching/training (manual or delegated), player interactions, academy/reserve pathways, scouting, media/news, board pressure and transfer authority. A good academy must *sometimes* produce viable first-team players—not guarantee stars.

**Match simulation:** CM-quality scoreline, key events, result, match report, league consequences, injuries, bookings, bans, fatigue and news. Owner-only players can press **Delegate/Instant Result** and continue; managing the team is opt-in. Not watching a match cannot suppress ordinary football resolution.

**Football release gate:** all relevant `entity → information → action → consequence → history` journeys are playable, or are delegated with an observable outcome; a complete season and multiple season rollovers pass without forced match watching or missing administration.

## 5. Owner, governance and personal wealth
**Owner entity:** identity, age/career, cash, investments, liabilities, shareholdings, loans receivable/payable, businesses, property, credit capacity, reputation, relationship graph, risk appetite and time commitments. Starting wealth is a scenario choice, not automatically equal to club cash. Cash is not net worth.

**Club ownership:** who owns what shares, voting/economic rights, board representation, acquisition price, financing/leveraging, shareholder agreements, minority rights, consortium governance, purchase/sale process, takeover interest and valuation. Multiple clubs/asset positions must respect rules that change with historical era. The owner may buy and sell, lose control, seek investors or invest through a holding company.

**Board and manager:** board objectives and budgets, recruit/poach/interview managers, agree authority and compensation, challenge or support, probation/review, dispute and dismissal, severance and reputation. Manager behaviour reflects temperament, results, pressure, promises and relationships. There is no one-click unrealistic 'perfect candidate' shortcut.

**Fans and public profile:** owner presence at home/away matches is a deliberate choice. Consistent presence, high-profile appearances, absences and unusual travel patterns influence perception—never as an automatic goals-scored buff. Memories of pricing, investment, loyalty, community and promises matter. Different owner types (ever-present, sporadic, absent, crisis-only) remain viable.

**Owner time:** interactions compete for attention. The player may delegate business or football tasks; reports summarise what has happened. Important opportunities and risks create choices, not compulsory repetitive administration.

## 6. Financial rules: no free £5m
A club has its own cash, accounts and obligations; the owner has separate personal or holding-company accounts.

| Transaction | Owner / holding-company ledger | Club ledger | Core effect |
|---|---|---|---|
| Equity injection | Cash down; investment carrying amount up | Cash up; share capital/premium or contributed equity up | No loan to repay; ownership/voting dilution if new shares |
| Interest-free shareholder loan | Cash down; loan receivable up | Cash up; related-party debt up | Repayment rules and solvency/approval conditions |
| Interest-bearing shareholder loan | Cash down; loan receivable up | Cash up; loan principal up | Scheduled interest expense/income; covenants and arrears |
| Bank loan | No automatic personal cash change unless guaranteed | Cash up; debt up | Interest, security, covenants and lender decision |
| Dividend | Cash up; investment may change in value | Cash down; retained earnings/distributable reserves down | Only legally and financially available distributions |
| Transfer / wage / stadium | No automatic owner payment | Cash, commitments and relevant expenses/assets change | Club must fund it or negotiate financing |
| Sell stake / repay loan | Cash received, asset relinquished / receivable reduced | Ownership ledger changed or cash/debt reduced | Tax/regulatory transaction events as era permits |

Minimum financial statements for each controlled club: profit and loss, balance sheet, cash flow, budget vs actual, transfer commitments, debt maturity, loan register, capital structure, liquidity runway, season projections. The owner dashboard shows liquid cash **separately** from net worth and the value of shareholdings.

An investment is a *two-sided recorded transaction* with immutable ID, posting date, method, source account, destination account, creditor/equity holder, authority, settlement status, ledger entries and reconciliation against the football engine's cash field. No hidden double posting, stale button, or free capital. Club cash is updated in the football engine through one controlled adapter, not by a rival competing cash simulation.

The existing F12 'Invest £5m' button is explicitly **experimental and unsafe as production economics**. It is a technical bridge test; do not promote it into the release as a permanent source of wealth.

## 7. Stadium, academy, commercial and external worlds
**Academy:** recruitment catchment, coaching, scouting, facilities, youth development, retention, educational/community links, age/level appropriate pathways, first-team opportunities and sell-on strategy; gradual, probabilistic and influenced by managerial decisions.

**Stadium and facilities:** capacity, demand, ticket yield, safety/compliance, redevelopment phases, funding and planning permission, training facilities, maintenance, event use, property value, local authority/community impact and construction risks.

**Commercial:** sponsorship, apparel/licensing, broadcast distribution, retail/merchandising, hospitality, ticketing, digital reach, contracts, partner reputation and commercial team autonomy; opportunity costs and crowd reactions.

**External businesses:** owner may invest in property, sport-linked ventures or other industries. Each has a genuine investment, revenue/expense, cash-flow and risk model, persistent but optional and linked to the main club. Do not turn early releases into five half-built management games.

**Historical world:** regulations, competition structures, media revenues, stadium norms, transfer windows, foreign-player rules, wage growth, sponsorship, technology and geography evolve on the correct timeline; major football events may change due to the simulation. Clearly distinguish researched historical baseline from alternative futures. Historic starting database must contain *correct-age footballers, clubs, finances, structures and rules* for the year selected. Current Bosman packaged choice from 2001/02–2025/26 does NOT provide a ready-made 1980 start.

## 8. Design language and interface
- Classic CM-style quick pages: Owner Home, Inbox, Club, Boardroom, Finance, Manager, Squad, Calendar, Academy, Infrastructure, Commercial, Portfolio, World and History.
- One global **Continue** control, with event batching and automatic summary of routine actions; no mandatory match viewing.
- Every main screen should answer: **Where are we? What changed? What decisions are mine? What happens next?** No inaccessible feature buried behind F12 forever.
- Financial views have clear provenance and source-of-funds, obligation and cash timing. Negotiation screens show pending offers and expected response windows, never silent disappearance.
- Alerts split: decision required / notable / report; meaningful autonomy guardrails avoid flood of clicks.
- Every major decision has confirmation, future-dated action if appropriate, audit trail and ability to inspect downstream effects (not universally undo them).
- Rich world yet accessible UI: legible type, fast keyboard/mouse navigation, scalable window, readable dashboards, sensible defaults.

## 9. Game difficulty, AI and fairness
- Manager, owner and player agents choose from constrained actions using goals, budgets, risk, temperament, relationships, history and imperfect information. They are **not** driven by scripted bonuses for the human player.
- Rivals must build and sell clubs, sign staff and players, employ managers, fund growth and respond to failure. AI uses the same accounting/authority rules as humans wherever practical.
- Reputational decisions, promises, negotiations and failed investments create persistent state.
- Scenarios: normal, difficult turnaround, financially wealthy benefactor, leveraged buyer, small-club builder; all obey the same rules.
- Football simulation is reproducible under fixed seeds for testing; results are not predetermined simply to reward owner spending.

## 10. Launch scope and later expansion
The **first integrated playable internal release** uses the already working 2001 football kernel as its technical fixture, then delivers an end-to-end owner loop: credible football season, separate owner identity and finances, one club acquisition/control route, manager appointment/oversight, authorised club funding, owner attend/delegate matchdays, finance reports, save/reload, persistent world consequences and a meaningful season-end.

The **first public-ready complete release** additionally requires the signature ~1980 starting world and its historical rules/data to be verified; multi-season progression and the football-completeness gate are mandatory. Ambitious optional mini-world depth, multiple simultaneous clubs and wide commercial-property mechanics can deepen after that rather than dilute core quality.

Project planning cannot silently reinterpret 'first playable' as 'finished'. The distinction must remain explicit.

## 11. Out of scope for the first playable release
- Photorealistic 3D match graphics, manual player movement or an FM26-style tactical complexity arms race.
- Recreating every property/retail/business vertical before football and finance are complete.
- Multiplayer and mobile-first UI.
- Promising 1980 databases are ready when the current source package only covers 2001 onwards.
- Monetisation assumptions or distribution of third-party copyrighted assets before permissions/legal review.

## 12. Product success and failure conditions
**Player success:** A new player can acquire/control a football club, appoint or retain a manager, set a credible strategy, make *funded* decisions, attend or skip matches, see real consequences, save/quit/return, and manage several seasons without being forced into the manager role.

**Engineering success:** no double-counted money; no lost decisions; unique transaction/event identities; no unhandled unsupported football action; deterministic replays/fixtures in tests; Mac build + install + fresh save + migrated save all tested; automated compatibility comparison before changing the football kernel.

**Failure:** a polished finance modal with no balance-sheet truth, a fictional 1980 start using 2001 data, owner decisions disappearing on reload, unresponsive contract negotiations, or a match that can only resolve by manually viewing it.

## Governing documents
- [Architecture and data contracts](ARCHITECTURE.md)
- [Roadmap and integrated acceptance gates](DELIVERY_PLAN.md)
- [Existing-code audit and technical risks](CODEBASE_AUDIT.md)
- [Decision register](DECISION_REGISTER.md)

**Change control:** game rules and system contracts are designed together before coding; use tests to prove one complete game journey per release, not one isolated button. Product vision above is the baseline, not a request for further user interviews.
