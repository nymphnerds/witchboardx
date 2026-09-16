# WitchboardX shared insert returns: CPU refinement handoff

**Date:** 2026-09-16. **Working branch:** `witchboardx-shared-returns` at `cdab0dde46251df60703ab735ae8975b2f4643da` in `/home/nymph/DistingNT/WitchboardX-shared-returns`.

## Current tested candidate

- User-reported disting NT setup: 44.1 kHz, Poly Res enabled, ten-channel WitchboardX preset, one stereo iPad insert on Drums ST / route E. The saved preset currently has an 18.2 ms (`182` tenths) return offset on route E.
- CPU reading: **23% with the insert return offset at 0 ms; 28% with it at 18.2 ms**. This is the latest hardware reading, for the object built from `cdab0dd`.
- Normal object filename: `plugins/WitchboardX.o`, SHA-256 `74eb037ea5c5134a2c9b9f8ebe6594f272631997b83e790c4cbbecc1270dfc5e`. An identical copy is in the existing `build/WitchboardX-cdab0dd-measured-23-28.o` file. The binary is ignored by Git; the source commit is the durable reference.
- The user subsequently confirmed that this candidate **runs without cutouts with Poly Res enabled** at the tested insert offset. The exact run duration was not supplied. Earlier branch objects eventually cut out; preserve this working object for comparison before trying another CPU change.
- `make verify` passed: host routing, gain, four-send, sidechain, timing, preset migration tests at 32/44.1/48/96 kHz, plus ARM object inspection. Host tests do not establish NT stability.

## Architecture and controls to preserve

- This branch has ten channels, six physical **insert routes**, and four **send FX** paths. Do not conflate inserts with send FX returns.
- The old per-channel negative offset page was removed. **Insert route** and **Insert return offset** editors are on Final Outputs. There is no send FX return offset in this version.
- **SC Lookahead remains in Sidechain/Master. Bypass Offset remains in Final Outputs.** These are separate user controls feeding one coordinated timing calculation. Changing an insert return offset must not change the SC Lookahead parameter.
- The highest offset among active insert routes sets the common insert reference. Dry Main, dry Bypass, and the active SC key follow that reference; each deferred insert return gets `reference - its route offset`. Main then receives its configured SC lookahead; Bypass receives its configured Bypass offset/auto-follow timing. This keeps the relative SC lookahead constant when an insert offset changes.
- A shared physical insert return is read once. Its contribution to a send FX path uses the highest wet amount among contributing channels. No audio-loop allocation.
- Serial Insert 1 + Insert 2 paths with different external latencies remain insufficiently device-validated; the route offset is treated as effective latency of the final return.

## What the measurements isolated

| Build / condition | NT Witchboard CPU | Interpretation |
| --- | ---: | --- |
| `9429ea3`, offset 0 / 18.2 ms | 31% / 39% | Timing branch and inactive send work still expensive. |
| Diagnostic with shared return timing compiled out, offset 0 | 31% | Idle timing bookkeeping was not the main 0 ms cost. Diagnostic was not a feature build. |
| Diagnostic processing only send FX 1, offset 0 | 23% | Skipping idle send FX 2–4 recovered roughly eight points in this preset. Diagnostic was not a feature build. |
| `f3e5254`, full features, offset 0 / 18.2 ms | 23% / 30% | Dynamic active-send lists retained all four sends and the 23% idle result. |
| `cdab0dd`, full features, offset 0 / 18.2 ms | **23% / 28%** | Stable shared-return mix cache, route bitmasks, and settled delay path saved about two more points with offset active. |

The latest active offset still costs roughly five CPU points. In the single route E case, its own software return delay is zero because it sets the maximum reference; the active work is mainly the common Main/Bypass/SC alignment ring and deferred-return routing. A delay length of 18.2 ms is not intrinsically a five-point CPU operation. The ring currently stores five float signals per sample in DRAM, and route work also switches on when the offset becomes positive. Do not assert which dominates without an NT isolation test.

Host preset-shaped 24-frame timing improved from about 1.90 to 1.66 microseconds per block with the active offset between `f3e5254` and `cdab0dd`. The host does not predict the NT meter reliably; use it only to reject clear regressions.

## Code changes in the current candidate

- `f3e5254`: per-channel active send indices and active FX return indices skip zero-level sends and disconnected FX returns in the audio loop. All four remain available, including live fade activation.
- `cdab0dd`: deferred route use is a bitmask instead of per-channel and per-frame boolean arrays; stable shared-return output/send levels are computed once per audio block; moving channels retain the dynamic path; the settled five-signal insert delay has a direct read/write path. The audio outputs are skipped when the collected channel signal is zero.
- Relevant code is `plugins/Witchboard/Witchboard.cpp`; focused tests are `tests/WitchboardSendsTest.cpp` and `tests/WitchboardInsertTimingTest.cpp`.

## Next session

1. Treat `cdab0dd` as the user-confirmed cutout-free, 23%/28% reference. Keep its measured object available for A/B tests; the duration of the successful run is unspecified.
2. If stable, isolate the remaining active-path cost with one controlled change at a time. Compare route collection against the five-signal DRAM delay ring while holding the same preset, sample rate, and Poly Res state. Preserve signal alignment and live fade behavior; avoid another broad latency rewrite.
3. Measure 0 and 18.2 ms after each candidate. Reject any candidate that raises the 23% idle baseline or causes earlier cutouts.
4. Hardware-validate two channels sharing one insert return, highest active route offset, SC Lookahead, Bypass Offset, live offset changes, and send FX levels independently. Host tests cover these paths but are not a substitute for sustained NT playback.

## Workspace guardrails

- Stable main checkout: `/home/nymph/DistingNT/Witchboard-master`, branch `main` at `4eb5eae`; it is clean and must remain untouched.
- The working branch's `presets/WitchboardX.json` has pre-existing user edits. Do not overwrite, restore, or stage it while working on CPU code.
- Do not create another checkout directory. Work in the existing shared-returns branch. `make verify` rebuilds `plugins/WitchboardX.o`; after a diagnostic, restore the normal filename to the intended test object before handing it to the user.
- Earlier 12-channel public reference is `nymphnerds/witchboard` tag `v1.1.2`: five insert routes, two send FX paths, no delay/sidechain/master-filter engine. The user's roughly 20% reading is useful context, but it is not an isolated feature comparison.

Earlier design and failed-build context: [shared-return rebuild from main](SHARED_RETURNS_REBUILD_FROM_MAIN.md) and the handoffs in the `WitchboardX-shared-returns-simple` checkout.
