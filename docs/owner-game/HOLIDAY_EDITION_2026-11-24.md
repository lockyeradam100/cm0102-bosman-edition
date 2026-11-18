# Owner Game — Holiday Edition (Mac)
## Playable-first release gate | 24 November 2026

**Status 18 November:** release candidate under Mac integration testing. This is a
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

### Download and install
1. Open the successful Owner Game **Mac Build** run in GitHub Actions.
2. Find **Artifacts** at the bottom of that run and download
   **Owner-Game-Holiday-Mac** (or **Owner-Game-Bosman-Mac** for the fallback).
   GitHub may require signing in. Extract the GitHub artifact ZIP first.
3. Open the included **.dmg**, then drag
   **CM 01-02 Bosman Edition.app** to your Applications folder.
   If the candidate provides an app ZIP instead, unzip and move the complete
   application to Applications. Do not copy the binary by itself.
4. Before replacing an existing application, close the running game.
   **Back up your save folder**:
   `~/Library/Application Support/cm0102`. It is separate from the app.
   Keep your previous working Mac app until the replacement is tested.
5. Open the application. If macOS blocks the unsigned/ad-hoc signed hobby
   build, check **System Settings > Privacy & Security > Open Anyway**
   after attempting to open it; confirm only if you trust this build.
6. Start a **new test career** first. Verify the club and game date, save,
   quit, reopen and reload. Only then continue a cherished career.

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
