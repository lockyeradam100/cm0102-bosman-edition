# Owner Game — Holiday Edition (Mac)
## Playable-first release gate | 24 November 2026

**Status 23 November:** release beta on native Mac; full matchday delegation is
experimental and not yet verified. The native app has passed real Arsenal
create/save/reload checks, but an owner-only match is not yet certified.
This is a
*playable CM01/02 + Owner Game prototype*, not the completed football ownership
sandbox described in the long-term programme plan.

**Release deadline:** make a passing build available for installation **by
Tuesday 24 November 2026**, with a preferred Monday 23 November release-candidate
checkpoint. Preserve the latest passing package even if a newer experiment fails.

### Known-good fallback
- GitHub Mac build **43899286537**, commit **5a43a4d6**, PASSED:
  https://github.com/lockyeradam100/cm0102-bosman-edition/actions/runs/43899286537
- Its 90-day GitHub Actions artifact is named **Owner-Game-Bosman-Mac** and
  contains a complete installable application DMG and self-contained app ZIP.
- Immutable fallback source branch: `owner-game-holiday-known-good`.
- This build passed native Mac compilation, real Arsenal start/save/reload,
  native contract offer/save/reload, one-week contract lifecycle observations
  and non-mutating manager-market parity. A diagnostic or pending offer does not
  prove a successful new manager appointment or a signed player deal.

### Release acceptance (all mandatory)
- [ ] One downloadable **complete Mac application** (DMG preferred; complete
      ZIP fallback), including game data, no Windows executable or manual patching
- [ ] Launch on native Apple Silicon with no Wine or Windows VM required
- [ ] Start a new Arsenal 2001 career and advance gameplay without a crash
- [ ] Save a native .sav, quit, relaunch, and continue the saved Arsenal career
- [ ] Opening the Owner Game overview and Manager Desk does not break CM gameplay
- [ ] An authenticated manager-delegation request is consumed by native CM,
      cancellation before kickoff works, and both are safe from duplicates
- [ ] Delegate and return to human control after a real CM match result where
      proven by native test; otherwise label the feature **experimental**
- [ ] Source-level parity, native contract and observer regression checks PASS
- [ ] Verify package artifact exists and is not expired, and offer a direct link
      to the successful GitHub Actions run for downloading it
- [ ] Protect the user's existing Arsenal career; no migration or save-folder
      deletion, no personal save or private game data in the repository

**Never mark an unverified feature as complete**. If a new test fails, repair it
or deliver the last known-good binary; the user must have a playable Mac game
for the holiday. The broader owner-game P1–P8 roadmap remains intact.

### Play in the morning — a football-first owner challenge

The newer builds may appear as **Owner-Game-Holiday-Playable-Mac** in GitHub
Actions even if the overall workflow has a red failure (the experimental
full-match automation). The playable beta is published only after native
build, create/save/reload, and manager-command checks pass. The **overall
red workflow is a warning that owner-only delegation is NOT certified**.
Check the actual artifact and README before installing.

**Safest installation:** keep your previous working application and Arsenal
save. Create a new Desktop folder such as `Owner Game Holiday Test`; place
the extracted application inside it. Do **not** drag over the existing
Applications copy. The two applications may still use the SAME CM save
folder, so back up `~/Library/Application Support/cm0102` first and save
your new career under a **different save name**.

**Football-first play path:** start a new 2001/02 club career, press
`Continue Game` for normal days, use CM's *original Go on Holiday*
(available from the green menu under the manager's name) when you want the
existing AI to handle squad selection, routine duties and games, and use
CM's own *Return from holiday* control when you want to interact again.
This native holiday path is **not the same feature** as Owner Game
`Delegate Next Match`; do not assume the experimental one-match feature
works or survives a save/reload. If a transfer bid or squad-number request
stops progression, deal with it in the native CM interface or continue
using the original holiday feature. Save periodically with a distinct
filename. The owner overview `Cmd-Shift-O` displays the linked club
identity, date and real club balance. Its finance injection is a sandbox
test — avoid it in your long career.

**For now, this is a playable football game with an early ownership
interface**, not a fully developed chairman career. Owner-only hiring,
manager employment, personal fortune, match attendance choices, owner
interviews and the 1980 start are still to be built.

### Download and install
1. Open the successful Owner Game **Mac Build** run in GitHub Actions.
2. Find **Artifacts** at the bottom of the run and download
   **Owner-Game-Holiday-Playable-Mac** for the latest beta, or
   **Owner-Game-Holiday-Mac** after a fully passing run
   (**Owner-Game-Bosman-Mac** for the known-good fallback).
   GitHub may require signing in. Extract the GitHub artifact ZIP first.
3. Open the included **.dmg** and copy **CM 01-02 Bosman Edition.app**
   to a new Desktop test folder; do not overwrite your existing installation.
   If the candidate provides an app ZIP instead, unzip the complete application
   into your new test folder. Do not copy the binary by itself.
4. Before replacing an existing application, close the running game.
   **Back up your save folder**:
   `~/Library/Application Support/cm0102`. It is separate from the app, but
   both installs can use that same save folder.
   Keep your previous working Mac app until the replacement is tested.
5. Open the application. If macOS blocks the unsigned/ad-hoc signed hobby
   build, check **System Settings > Privacy & Security > Open Anyway**
   after attempting to open it; confirm only if you trust this build.
6. Start a **new test career** first with a unique save name. Verify the club
   and game date, save, quit, reopen and reload. Only then continue a cherished career.

### Owner Game controls and what they really do
- **Cmd + Shift + O**: open Owner Game overview on Mac; **Fn + F12** also
  works if your Mac sends F12 (the function key often controls volume).
- **Manager Desk**: shows the current native club manager/controller and
  allows **Delegate Next Match**, where supported. Continue the game to
  apply the one-shot, club-bound command through native CM. Cancel a pending
  delegation before the manager takes over. Native CM decides matches and
  returns control after the fixture when that pathway functions.
- **Attend/manage a match**: do not delegate. Use CM's normal football
  management and match screens.
- **Invest £5m** is currently **a sandbox finance test**, not a real
  shareholder loan: it changes native club funds without deducting from an
  owner bank account. Avoid treating it as a finished financial feature.

### Manager-run week (experimental, 24 November candidate)

In Manager Desk, **Manager runs 7 days** requests a bounded native holiday.
CM's own AI handles the football and other decisions allowed by its current
holiday settings; the owner can intervene via CM's Return from holiday.
At the seven-day boundary the game attempts to restore human control.
This is not a separately employed manager, not a full owner inbox, and not
a persistent owner policy. Review native holiday settings before using it.
Until the native week gate passes, use this only on a disposable career.
Do not save or reload mid-week: native holiday persists but the new
seven-day marker does not. You can always use native Return from holiday.
No money is injected; use the existing club finances and review fixtures,
results and news when the week ends.

### Known limitations — not blockers for a holiday football build
- Hiring, poaching, dismissing or compensating managers **as owner** is not
  complete. The native CM manager market itself continues to exist.
- Separate personal owner wealth, acquisitions, equity/loans, accounting,
  academy, property and business mini-worlds are not playable yet.
- Delegation state is currently *process-local* and does **not** reliably
  survive saving/reloading between arming and kickoff. Finish or cancel
  a delegated match before quitting until its persistence gate passes.
- A genuine 1980/81 start is future scope; the base Bosman engine offers
  seasons starting in 2001/02 onward.
- No claim is made about an entire season of ownership gameplay or all
  football management parity until those full regression gates pass.

### Release operations
Use `owner-game-active-dev` for everyday engineering. Advance
`owner-game-offer-roundtrip` only by **safe fast-forward** of a coherent tested
milestone; this triggers expensive native Mac integration. Do not force push.
After a passing Mac release, record the precise commit, workflow run and
artifact name above. Prioritise user playability and reliable installation
over additional features after Monday 23 November. The two daily scheduled
engineering sessions should inspect/repair the release gate and provide
verified result links, not spend the week on documentation-only changes.
