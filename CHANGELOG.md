# Changelog

## v1.0.0 — first WitchboardX release

WitchboardX is a separate NT Gallery plugin using GUID `WtbX`; the original
Witchboard retains GUID `WtC1` and its own version history. This is a normal
release, not a prerelease. It requires disting NT v1.19 beta for DRAM cold-code
and serialisation support.

Includes 11 stereo channels, trigger ducking, Main lookahead, Bypass Offset,
master filter, serial inserts, FX sends and the aligned example preset.
The v1.34–v1.37 entries below are implementation handoff revisions, not
WitchboardX public release versions.

## v1.37 — eleven channels, output-page alignment and locked defaults

- Support and initially select 11 stereo channels: 234 of 241 parameters, with
  seven spare. Existing channel and global parameter identities remain intact.
- Move Bypass Offset to Final Outputs immediately after Bypass R. Retain the
  stereo delay and auto-follow behaviour, including preset-load protection.
- Set factory SC Depth to 69% and Curve to +20; retain Sidechain Off, unassigned
  trigger, 6 ms Lookahead, approximately 300 ms Length and Smooth 4.
- Set factory Filter Q to 10, LP limit to 20% and HP limit to 70%, without changing
  ranges or DSP. Existing saved values are preserved.
- Add four-rate channel-11 Main/Bypass/insert/FX and preset-restoration tests,
  and extend the preset converter to 11 channels. Hardware loading remains pending.

## v1.34 — trigger ducker and bypass alignment

- Remove all nine master EQ parameters and DSP, and the old sidechain modes,
  Attack/Hold/Release/Makeup and power-curve implementation.
- Add a JoyDuck-derived normalized trigger envelope, logarithmic 50–2000 ms
  length, signed curve, 0–200 ms linear-ramp smoothing and linear VCA depth.
- Add 0–10 ms Main lookahead and 0–100 ms Bypass Offset with 0.1 ms controls,
  DRAM stereo rings and 5 ms live tap crossfades in Split/Sum/Insert modes.
- Keep effective Bypass Offset visible and saved, follow mapped Lookahead,
  suppress load-time auto-follow and retain manual trim through range clamps.
- Preserve ten direct channel strips, master filter and final gain. Final
  parameter count is 219 (69 globals plus 150 channel parameters).
- Replace old EQ/hold tests with four-rate envelope/delay/routing/load tests;
  provide a v1.34 preset converter that preserves compatible mappings.
- Hardware audition remains pending.

## Unreleased

- Updated sidechain envelope defaults from the user's auditioned preset: Depth
  80%, Attack 0, Hold 100 ms, Release 140 ms, Curve -200, Makeup 0 dB.
  Preset contents and default enable/key routing remain unchanged.

- Added SC hold (0–500 ms) between Attack and Release on the master page,
  stored as the last global parameter; 229 parameters at 10 channels.
- Trigger/Gate attacks now use smoothstep at every duration, with a 1 ms
  minimum at Attack 0 and continuous gain on retriggers. No lookahead added.
- Converted the supplied preset to Attack 0, Hold 143 ms, Release 46 ms and
  Curve 0, retaining its 75% depth, channel settings and MIDI mappings. These
  defaults approximate the old 125 BPM envelope; hardware listening is pending.
- Host tests cover attack shape, Hold timing, retriggers and held gates at
  32/44.1/48/96 kHz; existing EQ/routing/gain and ARM checks pass.

- Implemented the 10-channel cap and explicit 241-parameter guard; the cap-only
  build passed host tests and ARM inspection at 227 parameters.
- Added channel gain up to +6 dB and a final Master Gain (-12 to +6 dB,
  default 0 dB, 10 ms smoothing). Current total: 228 parameters at 10 channels.
  Master Gain scales final Main/Bypass outputs, or the return in Insert mode.
- Aligned `presets/Tricky plastic time.json` for the extra global parameter,
  preserving channel values, MIDI mappings and names. Presets remain outside ZIPs.
- Gain/routing tests pass at 32/44.1/48/96 kHz; ARM inspection confirms the seven
  cold functions remain in DRAM with no new runtime dependencies. Hardware
  gain/listening verification remains pending. X2Buss and Dynamics3 are not
  included in this change; the existing filter and three-band EQ remain.

- Hardware smoke check (2026-09-07): user confirmed `WitchboardX.o` loads
  and seems fine. Full phase-1 acceptance checks are not yet recorded.

- Documented the first Witchboard v1.19 DRAM-placement pass: move cold setup,
  preset and UI functions such as `constructWitchboard()`, `serialise()`,
  `deserialise()`, `parameterString()`, `parameterUiPrefix()`,
  `calculateRequirements()` and `pluginEntry()` to DRAM, while keeping the
  real-time DSP path in fast code memory. Current ARM inspection suggests
  roughly 3 KiB of fast-code recovery, with `constructWitchboard()` alone at
  about 2 KiB.
- Added disting NT v1.19 beta development guidance for the new
  `_NT_DRAM_SECTION` API, which places selected plug-in functions in the
  `._nt_dram` section. Witchboard should use it for cold/setup/UI/preset code
  while keeping the real-time DSP path in fast code memory.
- Reorganised public documentation: current usage/setup remains in `README.md`; older branch and prototype history has been moved to `CHANGELOG.md`.

### Current `WtSF` state

- The current factory is `Witchboard SC Filter` with GUID `WtSF`.
- The current release object is `Witchboard-SC-Filter.o`.
- The sidechain controls are now `Sidechain`, `SC key input`, `SC key mode`,
  `SC depth`, `SC attack`, `SC release`, `SC curve`, and `SC makeup`.
- Added `Gate` as a sidechain key mode alongside `Trigger` and `Audio`.
- Current sidechain defaults are depth `90%`, attack `0 ms`, release `120 ms`,
  curve `-120`, and makeup `+2 dB`.
- Added the master SVF filter with `Filter enable`, `HP limit`, `LP limit`,
  `Filter Q`, and `Filter sweep`.
- `Filter sweep` uses negative values for low-pass, `0` for centre/bypass, and
  positive values for high-pass.
- The current source has `68` global parameters and `15` parameters per channel,
  for `248` plug-in parameters at the current 12-channel maximum.
- The earlier `WtDk` / `Witchboard Duck` entries below are retained as the
  development history of this branch.

- Added the experimental `WtDk` / `Witchboard Duck` layout.
- Replaced the old Main insert send/return block with an internal ducker on the
  Main path only; Bypass skips ducking and can rejoin at `Master output`.
- Added `Ducker makeup` with a `0..6 dB` range, applied only after ducking on
  the Main path.
- Removed the ducker `Hold` control. Hold-like pump shape is now handled by
  negative `Release curve` values.
- Current ducker defaults are aligned to the local performance preset:
  threshold `-60 dB`, ratio `1.50:1`, release `250 ms`, curve `-100`, makeup
  `+2 dB`.
- Current ducker ranges are threshold `-60..24 dB`, ratio `1.00..20.00`,
  release `100..800 ms`, curve `-200..0`, makeup `0..6 dB`.
- Changed `FX Send 1 mix` and `FX Send 2 mix` to an overlap-style dry/wet
  control.
- Dry now stays at full level from `0..50%` while wet fades in.
- Wet now stays at full level from `50..100%` while dry fades out.
- This replaces the old normalized cubic crossfade, which could feel like a
  centre dip with real FX returns.
- Updated the README to explain why Witchboard exists as a performance routing
  matrix, and to document the overlap FX-send behaviour.

## 1.1.1

- Cleaned up per-instance UI page storage so differently sized Witchboard
  instances do not share mutable page layout data.
- Hardened the host test memory helper for zero-byte DRAM allocations.
- Added regression coverage for simultaneous 4-channel and 12-channel page
  layouts.

## 1.1.0

- Expanded Witchboard from fixed insert routes to five generic assignable
  routes.
- Added per-channel Insert 1 and Insert 2 slot assignments.
- Preserved optional repeat protection so one channel can deliberately use the
  same route in both insert stages when protection is Off.
- Made SRAM allocation scale with the configured channel count instead of
  reserving all 12 channels for every instance.
- Reused static channel parameter labels with the official NT UI-prefix callback.
- Removed the old output-mode parameters; Witchboard outputs now always Add.
- Reduced the 12-channel parameter count to 241 so 12-channel respec works on
  the NT.
- Renamed the compiled `Radiant mix` parameter to generic `FX Send 1 mix`.
- Kept insert button behaviour as four states: Dry / Slot 1 / Slot 2 / Slot 3,
  using NT MIDI Mapping `Min = 0`, `Max = 4`.
- Fixed native/MIDI-mapped insert route changes lagging behind until another
  channel parameter, such as Gain, was nudged.
- Added host audio tests for routing and duplicate-return prevention.


## Historical Development Notes

The following material was moved from the README so the public README can stay focused on the current plug-in, setup, routing and controls. It is preserved here as development history.

## Why This Branch Exists

This branch adds an internal sidechain ducker and end-of-chain routing to
Witchboard.

The goal is simple: one Witchboard instance should be able to act as the main
routing matrix, the compressor-bypass mixer, the sidechain pump, and the final
stereo insert point. That means a patch can stay readable instead of needing an
extra "end Witchboard" or a feedback-prone external compressor routing helper.

In the old patch shape, the main Witchboard sent some channels through a
sidechain compressor and sent other channels around it on a bypass lane. A
second Witchboard instance then had to sit at the end of the patch to sum those
two lanes back to outputs 1/2.

This branch lets the main Witchboard do that final job itself:

- `Main` is the normal mix path.
- `Ducker` can reduce only the Main path from an external key bus.
- `Bypass` skips the ducker, so kicks or other sounds can avoid the pump.
- `Master output` decides what happens after Main and Bypass meet again.
- `Master output: Insert` sends the whole final stereo mix out and brings it
  back, useful for an iPad, computer, DJ-style processor, or mastering chain.

The intended before/after is:

![Before: the patch needed an extra end Witchboard to rejoin the main and bypass paths.](assets/patch1.png)

![After: the main Witchboard owns the main insert, bypass rejoin, and master insert routing.](assets/patch2.png)

```text
Before:

Main Witchboard -> sidechain duck -> End Witchboard -> outputs 1/2
       bypass ------------------------^

After:

Main Witchboard -> internal ducker -> Master insert -> outputs 1/2
       bypass --------^
```

## What's Changed From v1.0.0

Compared with the old main-branch release, `v1.1.1` expands Witchboard from the
original fixed insert layout into a 12-channel routing matrix with five
assignable insert routes. Each channel now has per-channel Slot 1/2/3 route
assignments for both insert stages, so the same four-state control can mean
different hardware paths on different channels.

The 4-state insert selectors now work cleanly from buttons, faders, knobs, or
CV-mapped controls. Outputs are fixed to Add so the plugin stays within the NT
parameter limit, and the public release now ships as `Witchboard.o` plus
`Witchboard.zip`. `v1.1.1` keeps that behavior and cleans up per-instance page
storage so differently sized Witchboard instances do not share mutable page
layout data.

The next update changes the FX send mix from the old normalized crossfade to an
overlap-style dry/wet control. The dry signal now stays full while wet comes in,
then the wet signal stays full while dry fades out.

## Earlier WtDk Experiment

The following `Latest Change` section is preserved from the earlier `WtDk`
prototype stage. It is historical context, not the parameter list for the
current `WtSF` build above.

## Latest Change

This branch currently ships the `WtDk` Witchboard Duck experiment. It replaces
the old Main insert send/return block with an internal ducker on the Main path
only, adds a final Master output stage, and keeps Bypass outside the ducker.

Current ducker controls:

| Parameter | Range | Default | Notes |
|---|---:|---:|---|
| `Ducker` | `Off..On` | `Off` | Enables Main-path ducking. |
| `Ducker key input` | bus | `None` | External clock, trigger, envelope, or audio key bus. |
| `Ducker key mode` | `Trigger / Audio` | `Trigger` | Trigger mode launches from key edges; Audio mode follows key level. |
| `Ducker threshold` | `-60..24 dB` | `-60 dB` | Trigger detect/sensitivity. |
| `Ducker ratio` | `1.00..20.00` | `1.50` | Compressor-style reduction amount. Raw preset values are scaled by 100. |
| `Ducker release` | `100..800 ms` | `250 ms` | Recovery time. |
| `Release curve` | `-200..0` | `-100` | Negative values create a hold-like/pump shape. |
| `Ducker makeup` | `0..6 dB` | `2 dB` | Applied after ducking to Main only. |

There is no `Hold` parameter. The tuned pump uses `Release curve` for the
hold-like feel.

The current local performance preset is:

```text
/home/nymph/DistingNT_presets/Unconscious choice.json
```

Its current WtDk block is:

```text
[1, 28, 0, -60, 150, 250, -100, 2]
```

That maps to Ducker On, key bus 28, Trigger mode, threshold `-60 dB`, ratio
`1.50:1`, release `250 ms`, curve `-100`, and makeup `+2 dB`.

## Parameter Count

Witchboard Duck uses `63` global parameters and `15` parameters per channel.
Serialized presets include the host common Bypass parameter before plugin
parameter 0, so a 9-channel WtDk preset block has:

```text
1 common host parameter + 63 globals + 9 * 15 channel parameters = 199 values
```

| Channels | Plugin parameters |
|---:|---:|
| 1 | 78 |
| 4 | 123 |
| 8 | 183 |
| 12 | 243 |

The current layout is designed so a 12-channel instance fits under the NT
parameter limit.

## Installation

Copy the object to the disting NT MicroSD card:

```text
Witchboard-Duck.o -> /programs/plug-ins/Witchboard-Duck.o
```

Then rescan plugins or restart the module.

This plugin is built against the official disting NT plugin API v13.
