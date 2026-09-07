# Witchboard

Witchboard is a routing mixer and serial patchbay plug-in for the Expert Sleepers
disting NT.

It is designed for performance patches where each source behaves like a normal
mixer channel, while also being able to move through hardware inserts, external
processors and shared FX from the NT parameter/MIDI-mapping system.

> The supported maximum is **10 stereo channels**.

## DSP Credits & Why They Were Chosen

Witchboard deliberately builds its core tone-shaping around small, high-quality
open-source DSP designs which suit the disting NT and, more importantly, sound
right for the instrument.

### Filter — Matthijs Hollemans / Cytomic SVF

- [Matthijs Hollemans — Cytomic SVF C++ implementation](https://gist.github.com/hollance/2891d89c57adc71d9560bcf0e1e55c4b)
- [Andrew Simper / Cytomic — *SvfLinearTrapOptimised2*](http://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf)

**Why Witchboard chose it:** the topology-preserving state-variable filter gives
the smooth, immediate and musical HP/LP sweep behaviour wanted from
Witchboard's DJ-style filter, while staying compact and practical on the
disting NT. The Witchboard implementation keeps the filter deliberately close
to this source design.

### Master EQ — Signalsmith Audio / Geraint Luff

- [Signalsmith Audio DSP](https://github.com/Signalsmith-Audio/dsp)
- [Signalsmith `filters.h`](https://github.com/Signalsmith-Audio/dsp/blob/main/filters.h)

**Why Witchboard chose it:** Signalsmith provides compact, high-quality C++
biquad designs with the peaking and shelf responses Witchboard needs and
careful behaviour near Nyquist. It gives the master EQ a strong tested
mathematical foundation without importing a heavyweight DSP framework.

### Per-channel Dynamics — Airwindows Dynamics3 / Chris Johnson

- [Airwindows Dynamics3 — release, explanation and demo](https://www.airwindows.com/dynamics3/)
- [Airwindows source repository](https://github.com/airwindows/airwindows)

**Why Witchboard chose it:** Dynamics3's Bezier-spline dynamics and unusual
`Inv/Wet` compression-to-expansion behaviour are exceptionally musical and
playable. It was chosen to make each Witchboard channel's dynamics feel like a
creative instrument rather than a generic utility compressor. Its four core
sound controls — **Threshold, Attack, Release and Inv/Wet** — also fit the
disting NT's strict parameter budget unusually well.

> **Development note:** Dynamics3 is the selected design for Witchboard's
> planned per-channel dynamics stage; it is not yet part of the current
> released build.

## Features

- Up to **10 stereo source channels**
- Two serial insert stages per channel
- Five assignable insert routes
- Two shared stereo FX sends
- Main / Bypass output paths
- Internal sidechain gain shaping on the Main bus
- DJ-style HP/LP sweep filter
- Split / Sum / Insert master-output modes
- Master external insert send/return
- Preset-specific route, FX and slot naming
- Parameter smoothing for live routing changes

## Signal Flow

Each source channel runs:

```text
Input L/R
  -> Gain
  -> Insert 1
  -> Insert 2
  -> FX Send 1
  -> FX Send 2
  -> Main or Bypass
```

The two output paths then enter the master section.

### Split

```text
Main -> Sidechain -> Filter -> Main L/R
Bypass -----------------------------> Bypass L/R
```

### Sum

```text
Main -> Sidechain --\
                     +-> Filter -> Main L/R
Bypass -------------/
```

### Insert

```text
Main -> Sidechain --\
                     +-> Filter -> Master Send L/R
Bypass -------------/                       |
                                             v
                                      external processor
                                             |
                                             v
                                      Master Return L/R
                                             |
                                             v
                                         Main L/R
```

Only the `Main` path is affected by the internal sidechain stage.

## Channel Controls

Each channel has:

- Enable
- Input L
- Input R
- Gain
- Insert 1
- Insert 1 Slot 1
- Insert 1 Slot 2
- Insert 1 Slot 3
- FX Send 1 mix
- Insert 2
- Insert 2 Slot 1
- Insert 2 Slot 2
- Insert 2 Slot 3
- Output path
- FX Send 2 mix

Unused channels can be disabled or left with no Input L bus assigned.

## Insert Routes

Witchboard has five assignable insert routes:

```text
Route A
Route B
Route C
Route D
Route E
```

Each route has a send, return and mono/stereo width configuration.

Each channel has two serial insert selectors. Each selector can be:

| Value | State |
|---:|---|
| 0 | Dry |
| 1 | Slot 1 |
| 2 | Slot 2 |
| 3 | Slot 3 |
| 4 | Slot 3 |

The extra top value keeps common four-position MIDI controls such as
`0 / 42 / 85 / 127` landing cleanly on Dry / Slot 1 / Slot 2 / Slot 3 when the
NT mapping range is set to `0..4`.

Each slot can point independently to Route A-E.

`Repeat protection` prevents the same route being used twice in series on one
channel. When protection is Off, the same route may deliberately be selected in
both insert stages.

## FX Sends

Two shared stereo FX paths are available:

- FX Send 1
- FX Send 2

Each channel has an `FX Send 1 mix` and `FX Send 2 mix`.

The mix control uses an overlap shape:

```text
0%        dry 1.0, wet 0.0
50%       dry 1.0, wet 1.0
100%      dry 0.0, wet 1.0
```

From 0-50%, dry stays full while wet fades in.

From 50-100%, wet stays full while dry fades out.

Each FX return can be assigned to Main or Bypass.

## Sidechain

The sidechain stage affects the `Main` path only.

| Parameter | Range | Default |
|---|---:|---:|
| `Sidechain` | Off / On | Off |
| `SC key input` | NT input bus | None |
| `SC key mode` | Trigger / Gate / Audio | Trigger |
| `SC depth` | 0..100% | 90% |
| `SC attack` | 0..50 ms | 0 ms |
| `SC release` | 5..800 ms | 120 ms |
| `SC curve` | -300..200 | -120 |
| `SC makeup` | 0..6 dB | +2 dB |

### Trigger

A rising key event launches the sidechain envelope.

### Gate

The key signal holds the ducked state while active and releases when the gate
falls.

### Audio

The key level is followed continuously and used to derive the gain reduction.

`SC makeup` is applied after the sidechain gain stage to Main only.

## Filter

The master filter is a state-variable DJ-style sweep filter.

| Parameter | Range | Default |
|---|---:|---:|
| `Filter enable` | Off / On | Off |
| `HP limit` | 0..100% | 100% |
| `LP limit` | 0..100% | 0% |
| `Filter Q` | 0..100% | 0% |
| `Filter sweep` | -100..100% | 0% |

`Filter sweep` works around a clean centre:

```text
-100 ... -1   low-pass side
0             filter bypass
+1 ... +100   high-pass side
```

The cutoff mapping is logarithmic from approximately 20 Hz to 20 kHz, clamped
below Nyquist.

`LP limit` and `HP limit` define the extremes reached by the negative and
positive sides of the sweep.

`Filter Q` maps from approximately `0.707` to `12`.

## Master Inserts & Output

`Master output` has three modes:

| Mode | Behaviour |
|---|---|
| `Split` | Main -> Main L/R; Bypass -> Bypass L/R |
| `Sum` | Main + Bypass -> Main L/R |
| `Insert` | Main + Bypass -> Master Send; Master Return -> Main L/R |

Master Inserts use:

- `Master send L`
- `Master send R`
- `Master return L`
- `Master return R`

## Output Paths

Every channel can choose:

- Main
- Bypass

Use Main for channels that should pass through the sidechain stage.

Use Bypass for channels that should avoid sidechain gain shaping and either stay
separate in Split mode or rejoin the master stage in Sum/Insert mode.

All Witchboard output buses are additive.

## Switching and Smoothing

`Switch fade` controls the transition time used for live routing changes and
channel smoothing.

```text
0..100 ms
default: 2 ms
```

Channel gain, insert switching and FX-send moves use this smoothing system.

## Preset Naming

Witchboard can store preset-specific display names for routes, FX sends and slot
states.

```text
witchboardNames.routes
witchboardNames.fx
witchboardNames.slots
```

These names affect the UI only; routing behaviour remains generic.

## Installation

```text
Name: Witchboard SC Filter
GUID: WtSF
Artifact: Witchboard-SC-Filter.o
```

Copy the built object to the disting NT MicroSD plug-in directory, then rescan
plug-ins or restart the module.

## disting NT v1.19 Beta / DRAM Code Placement

For development builds targeting the first disting NT **v1.19 beta**, the
Expert Sleepers API now supports placing selected plug-in functions in DRAM:

```cpp
_NT_DRAM_SECTION
void someColdFunction(...)
{
    ...
}
```

`_NT_DRAM_SECTION` maps the function to the `._nt_dram` section.

Witchboard should use this only for cold/setup/UI/preset code where appropriate,
while keeping the real-time audio path in fast code memory. This preserves
scarce instruction-memory headroom for the filter, 4-band EQ and planned
Dynamics3 processing.

API:
https://github.com/expertsleepersltd/distingNT_API

### Current Witchboard DRAM-placement plan

The current ARM build suggests roughly **3 KiB** of fast code can be recovered
conservatively by moving cold functions to DRAM first.

Primary candidates:

```text
constructWitchboard()
calculateRequirements()
serialise()
deserialise()
parameterString()
parameterUiPrefix()
pluginEntry()
```

`constructWitchboard()` alone is about **2 KiB** of ARM code and is the biggest
single cold-code target.

The real-time audio path remains in fast code memory, including `step()`,
filter/EQ processing, sidechain processing, coefficient updates used from the
audio path, and the future Dynamics3 kernel.


## Building

The Makefile expects the official disting NT API at `../distingNT_API` by
default.

```sh
make
```

To use another API location:

```sh
make NT_API_PATH=/path/to/distingNT_API
```

For the full validation/package path:

```sh
make package NT_API_PATH=/path/to/distingNT_API
```

The project includes:

- focused C++ host tests
- ARM object build
- object inspection
- release packaging

## Filter Credits

The master state-variable filter implementation is based on the C++
`StateVariableFilter` reference by **Matthijs Hollemans (`hollance`)**, which
implements the **Cytomic SVF** design by **Andrew Simper of Cytomic**.

Source:

https://gist.github.com/hollance/2891d89c57adc71d9560bcf0e1e55c4

Original Cytomic design paper:

http://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf

Witchboard adapts that filter approach for the disting NT and adds the master
DJ-style HP/LP sweep, HP/LP limits and Q control.

## Developer Notes: disting NT Memory

Useful memory figures from Expert Sleepers:

```text
ITC   64 kB
DTC   24 kB
DRAM  512 kB
```

These are implementation details rather than guaranteed API limits.

Plug-in object sections map approximately as follows:

```text
.text                 -> ITC
.data / .data.rel.ro  -> DTC
.rodata               -> DRAM
```

Per-instance memory requested through `calculateRequirements()` comes from the
larger global algorithm-memory pool rather than the 24 kB static DTC pool.

`NT_globals.workBuffer` lives in DTC and can be used for temporary scratch data
inside `step()`.

Built-in disting NT algorithms run mostly from flash/XIP, with selected hot
functions placed in ITC.

When diagnosing a `Not enough memory` error, inspect both:

- plug-in object section sizes
- per-instance memory requested by `calculateRequirements()`

The 11-channel support limit is based on hardware behaviour: a 12-channel
Witchboard instance fails to load with an out-of-memory error on the latest
disting NT firmware.

## Repository Layout

```text
README.md                  User documentation
CHANGELOG.md               Development and release history
plugins/Witchboard/        Single-file C++ implementation
scripts/                   Inspection, packaging and preset migration
tests/                    Host routing and EQ tests
assets/                    README images
docs/handoffs/             Ordered implementation handoff and memory findings
.github/workflows/         Build and release automation
Makefile                   Build, test, inspect and package entry points
```

The next stage follows [the ordered v1.19 implementation handoff](docs/handoffs/Witchboard_v3_ORDERED_IMPLEMENTATION_HANDOFF_v119.md).
The [solved memory investigation](docs/handoffs/Witchboard_NT_Memory_Allocation_Investigation_SOLVED.md)
records the confirmed parameter ceiling. These are development inputs; the
copied source is the starting baseline, not the completed handoff implementation.

`build/`, `release/` and compiled `.o` files are local generated artifacts and
are excluded from Git. Run `make verify` to check the source, or `make package`
to produce release files. The API checkout is discovered at `../distingNT_API`
or `../../distingNT_API`; use `NT_API_PATH=/path/to/distingNT_API` to override it.
