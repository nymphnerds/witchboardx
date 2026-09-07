# Witchboard v3 — Ordered Implementation Handoff

## Purpose

This handoff is intentionally ordered as a sequence of **small, independently
testable changes**.

Do **not** implement the entire document in one pass.

Each phase should be:

```text
change
-> build
-> inspect
-> hardware test
-> commit
-> only then continue
```

This keeps failures attributable to one change and gives a clean rollback point
after every stage.

## Fixed architectural facts

```text
Supported channel target:     10
Current 10-channel params:     227
Confirmed NT hard maximum:     241

After planned 4-band EQ:       231
After planned Dynamics3 UI:    237
Final spare public params:       4
```

The 10-channel cap is intentional. Do not spend effort recovering an 11th
channel unless the product direction explicitly changes.
# PHASE 1 — v1.19 DRAM placement only

**Goal:** free fast instruction memory without changing sound, parameter
count, channel count, routing, filter behaviour, EQ behaviour or preset
architecture.

This must be the first change because it gives a clean measurement of how much
fast-code headroom the existing Witchboard can recover before any new DSP is
added.

Create a dedicated commit/branch for this phase only.

## Platform / memory context

Useful Expert Sleepers developer information previously gathered for this project:

```text
ITC   ~64 kB
DTC   ~24 kB
DRAM  ~512 kB
```

These were described as implementation details, not guaranteed permanent API limits.

Static allocation and per-instance algorithm memory must not be conflated.

Per-instance memory requested in:

```cpp
calculateRequirements()
```

comes from the algorithm memory system and should not simply be added to the static 24 kB DTC figure.

`NT_globals.workBuffer` is temporary DTC scratch memory for `step()` and is not persistent.

Built-in algorithms run mostly from flash/XIP, with selected hot code in ITC.

## disting NT v1.19 beta: functions can now be placed in DRAM

The first disting NT **v1.19 beta** adds official support for placing selected
plug-in functions in DRAM.

The current API defines:

```cpp
#define _NT_DRAM_SECTION __attribute__ ((section ("._nt_dram")))
```

Use it immediately before a function definition, for example:

```cpp
_NT_DRAM_SECTION
uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
    ...
}
```

Expert Sleepers' own `examples/gain.cpp` now uses `_NT_DRAM_SECTION` on
`pluginEntry()`.

The supplied v1.19 beta firmware identifies itself as:

```text
Firmware v1.19.0beta
```

and contains loader support for:

```text
._nt_dram
out of space for DRAM veneer
```

So this is no longer speculative or merely upcoming: **v1.19 beta implements
DRAM function placement.**

### Witchboard policy

Use DRAM placement deliberately for **cold / non-audio-critical code** to reduce
pressure on scarce fast instruction memory.

Good first candidates include:

- `pluginEntry()`
- construction/setup helpers
- parameter/page-building helpers
- `serialise()`
- `deserialise()`
- `parameterString()`
- `parameterUiPrefix()`
- other UI/preset/helper functions that are not used in the real-time audio path

Do **not** move hot DSP code to DRAM by default, including:

- `step()`
- per-sample filter processing
- the Dynamics3 audio kernel
- anything called every sample or every block where execution speed matters

Coefficient/design helpers should be classified by actual call frequency. If a
helper is called from `step()` every block, treat it as hot until profiling
proves otherwise.

After applying `_NT_DRAM_SECTION`, inspect the object/linked result and
hardware-test the plug-in. The goal is to keep hot DSP in fast code memory and
move cold code into DRAM without changing sound or behaviour.

API reference:
https://github.com/expertsleepersltd/distingNT_API

Current example:
https://github.com/expertsleepersltd/distingNT_API/blob/main/examples/gain.cpp


The 10-channel limit should now be treated as an intentional supported product limit. The 11-channel build has 242 parameters and exceeds the confirmed 241-parameter maximum by exactly one. Runtime DRAM was not the cause.

---

## Witchboard v1.19 DRAM placement — current first-pass candidates

Based on the current Witchboard ARM object/source inspection, the recommended
first pass is deliberately conservative: move only cold/non-audio-critical code
to DRAM and leave the entire real-time DSP path in fast code memory.

### Strong candidates

| Function | Approx. ARM code size | Recommendation | Reason |
|---|---:|---|---|
| `constructWitchboard()` | ~2048 B | **Move to DRAM** | Largest cold-code win; construction/setup only |
| `deserialise()` | ~312 B | **Move to DRAM** | Preset load only |
| `serialise()` | ~260 B | **Move to DRAM** | Preset save only |
| `parameterString()` | ~328 B | **Move to DRAM** | UI formatting only |
| `parameterUiPrefix()` | ~76 B | **Move to DRAM** | UI-only helper |
| `pluginEntry()` | ~40 B | **Move to DRAM** | Official Expert Sleepers example already does this |
| `calculateRequirements()` | ~62 B | **Move to DRAM** | Construction/specification path only |
| `parameterChanged()` | ~56 B | **Optional / low priority** | Not audio-rate, but too small to matter much |

The most valuable single move is:

```cpp
_NT_DRAM_SECTION
_NT_algorithm* constructWitchboard(...)
```

because the compiler has inlined a large amount of setup logic into
`constructWitchboard()`, including parameter/page construction and runtime
initialisation. Moving that one function may recover roughly **2 KiB** of fast
instruction memory by itself.

A sensible first pass is therefore:

```text
MOVE TO DRAM
✅ constructWitchboard()
✅ calculateRequirements()
✅ serialise()
✅ deserialise()
✅ parameterString()
✅ parameterUiPrefix()
✅ pluginEntry()

OPTIONAL / LOW VALUE
◻ parameterChanged()
```

Expected first-pass fast-code recovery is roughly:

```text
~3 KiB
```

before measuring the linked result.

### Keep in fast code memory

Do **not** move the following to DRAM in the first pass:

```text
step()
processMasterEq()
advanceMasterEq()
prepareMasterEq()
makeFilterCoefficients()
advanceChannel()
shapedCrossfade()
sidechain envelope processing
filter sample processing
EQ biquad processing
future Dynamics3 audio kernel
```

Important: some functions which *sound* like setup are actually part of the
real-time path in the current Witchboard implementation.

In particular:

- `prepareMasterEq()` is called every block
- `makeFilterCoefficients()` is called from `step()` every block
- `advanceMasterEq()` is per-sample
- `processMasterEq()` is per-sample

Therefore they must be treated as hot DSP unless profiling later proves that a
different placement is safe.

### Validation after moving functions

After the first `_NT_DRAM_SECTION` pass:

1. rebuild Witchboard against the v1.19 beta API
2. inspect `.text` / `._nt_dram` / related section sizes
3. confirm the intended functions actually moved
4. compare fast-code usage before/after
5. load the plug-in on hardware
6. test preset save/load and UI callbacks
7. stress-test filter + EQ + sidechain processing
8. later repeat with Dynamics3 enabled once integrated

The purpose is to recover cold-code headroom **without touching the sound or
real-time execution path**.

## Phase 1 acceptance gate

Do not continue until:

1. Witchboard builds against the v1.19 beta API.
2. The intended functions appear in `._nt_dram`.
3. Fast-code usage is measured before and after.
4. Current Witchboard loads on hardware.
5. Filter, sidechain, inserts, routing and preset save/load behave exactly as before.
6. No hot DSP function was accidentally moved to DRAM.

Expected conservative fast-code recovery from the current object is roughly:

```text
~3.0–3.1 KiB
```

with `constructWitchboard()` alone accounting for roughly 2 KiB.
# PHASE 2 — Make 10 channels the intentional maximum

**Goal:** lock the product architecture to 10 channels and make the
241-parameter ceiling explicit in source/tests/docs.

This phase should contain no EQ redesign and no Dynamics3 work.

## 10-channel decision and parameter limit

The current v3 source has:

```text
77 global parameters
15 parameters per channel
```

and currently exposes:

```cpp
constexpr int kMaxChannels = 11;
```

Expert Sleepers developer Os has now pinpointed why the 11-channel build fails: the disting NT hard maximum is **241 parameters per algorithm**.

Current Witchboard counts are:

```text
10 channels:
77 globals + (10 × 15) = 227 parameters

11 channels:
77 globals + (11 × 15) = 242 parameters
```

Os replied to the reported 242-parameter build:

> "You're over the max number of parameters by 1, I'm afraid."

Therefore the usable maximum is **241**, and 242 is invalid.

The product decision is now to **cap Witchboard intentionally at 10 channels**, rather than spend parameter budget squeezing an 11th channel in.

Make:

```cpp
constexpr int kMaxChannels = 10;
```

the supported maximum.

This is now an intentional **parameter-budget/product-design limit**, not a runtime-memory workaround.

Update all dependent:

- specification limits
- channel page arrays/names
- tests
- parameter-count assertions
- release validation
- README references

Do not otherwise change the channel architecture.

## Current parameter count at 10 channels

```text
77 globals + (10 × 15) = 227
```

---

## Phase 2 acceptance gate

Confirm:

```text
10 channels = 227 parameters
```

and:

- 10 channels load reliably
- 11 channels are no longer exposed
- all channel pages/specification limits agree
- a compile-time guard prevents exceeding 241 parameters

Recommended guard:

```cpp
static_assert(kMaxParams <= 241,
              "disting NT supports at most 241 parameters per algorithm");
```

If Expert Sleepers exposes a named constant later, use that instead.
# PHASE 3 — Replace the master EQ

**Goal:** replace the existing 3-bell EQ with the agreed 4-band master EQ
while leaving the filter, sidechain and channel dynamics untouched.

Target after this phase:

```text
231 total parameters
```

Do not begin Dynamics3 integration until this phase is stable on hardware.

## EQ design and implementation details

The existing EQ is already a good-quality Signalsmith-derived peaking EQ implementation.

Do not throw away its useful framework.

Preserve where practical:

- sample-rate handling
- logarithmic frequency mapping
- parameter smoothing
- coefficient update strategy
- independent left/right filter state
- shared coefficients between stereo sides
- current safe-Nyquist handling
- current double-precision coefficient/state approach if retained by v3

The problem is the **topology**, not the basic implementation quality.

The master section should become:

```text
Low Shelf
Mid 1 Bell
Mid 2 Bell
High Shelf
```

Both Mid bands must be independently sweepable over the **entire available frequency range**.

There is no crossover and no artificial low-mid/high-mid split.

Mid 1 and Mid 2 are allowed to cross over each other freely.

---

Add one global parameter:

```text
EQ Enable
Off / On
```

Default:

```text
On
```

or preserve the most sensible behaviour for existing presets if backwards compatibility requires otherwise.

When EQ is disabled:

- bypass all four EQ bands
- do not reset unrelated DSP
- avoid unnecessary EQ processing if practical
- avoid clicks when toggling

Use the project's existing smoothing/bypass conventions where possible.

---

The finished EQ has 13 global parameters:

```text
EQ Enable

Low Shelf Freq
Low Shelf Gain
Low Shelf Q

Mid 1 Freq
Mid 1 Gain
Mid 1 Q

Mid 2 Freq
Mid 2 Gain
Mid 2 Q

High Shelf Freq
High Shelf Gain
High Shelf Q
```

The old EQ used 9 parameters.

The new design therefore adds:

```text
+3 for fourth band
+1 for EQ Enable
= +4 parameters
```

## New total at 10 channels

```text
227 current
+ 4
= 231 parameters
```

The confirmed disting NT hard maximum is:

```text
241 parameters per algorithm
```

So the 4-band EQ build at 10 channels leaves:

```text
241 - 231 = 10 spare exposed parameters
```

The API's `uint8_t` parameter indices can address 0–255, but that addressing range is **not** the usable runtime parameter capacity.

The 10-channel cap is deliberate: it leaves meaningful headroom for future controls instead of running the product at the 241-parameter ceiling.

---

Use the **same Signalsmith Audio DSP source family already used by the current EQ**.

## Primary source

Signalsmith Audio DSP repository:

https://github.com/Signalsmith-Audio/dsp

Relevant filter implementation:

https://github.com/Signalsmith-Audio/dsp/blob/main/filters.h

MIT licence:

https://github.com/Signalsmith-Audio/dsp/blob/main/LICENSE.txt

Author/project credit:

**Signalsmith Audio / Geraint Luff**

Preserve the required MIT attribution for substantially copied/adapted code.

---

Signalsmith's `BiquadStatic` source already contains the exact filter types required.

Use/adapt the equivalent of:

```text
Low Shelf  -> lowShelfDbQ(...)
Mid 1      -> peakDbQ(...)
Mid 2      -> peakDbQ(...)
High Shelf -> highShelfDbQ(...)
```

Relevant source:

https://github.com/Signalsmith-Audio/dsp/blob/main/filters.h

The same source defines:

```cpp
enum class BiquadDesign {
    bilinear,
    cookbook,
    oneSided,
    vicanek
};
```

Signalsmith documents `oneSided` as a bilinear-based design which adjusts bandwidth to preserve the lower boundary as frequency approaches Nyquist.

The current Witchboard EQ already uses/adapts this `oneSided` approach for the bell filters.

Keep that same design family for the shelves.

Do not mix unrelated shelf maths into the master EQ unless testing proves a real need.

---

The original Signalsmith source contains explicit low- and high-shelf coefficient equations.

The shelf paths are part of the same `configure()` implementation used by the peaking filters.

Therefore this is a natural extension of the current EQ, not a replacement with a different DSP family.

Conceptually:

```text
existing:
Peak -> Peak -> Peak

new:
Low Shelf -> Peak -> Peak -> High Shelf
```

The audio-rate cost increases by only one additional stereo biquad stage.

---

Use a **tried-and-tested musical master-EQ layout**, not a laboratory-style full-range layout for every band.

Keep logarithmic frequency control where appropriate and continue to derive all frequency limits from:

```cpp
NT_globals.sampleRate
```

with safe clamping below Nyquist.

Do not assume 48 kHz.

## Low Shelf

The Low Shelf should be deliberately constrained to the low-frequency region for broad weight/body shaping.

Recommended continuous range:

```text
approximately 30 Hz – 300 Hz
```

This follows the same practical philosophy as dedicated mastering/bus EQs which keep low-shelf turnover frequencies in a musically useful low-end region rather than allowing the shelf to roam across the whole spectrum.

Suggested default:

```text
~100 Hz
```

## Mid 1 Bell

Mid 1 should remain a wide-ranging, fully parametric bell for broad shaping or corrective work.

Recommended range:

```text
approximately 20 Hz – 20 kHz
```

subject to safe Nyquist clamping.

Suggested default:

```text
~400 Hz
```

## Mid 2 Bell

Mid 2 should use the same wide usable range as Mid 1.

Recommended range:

```text
approximately 20 Hz – 20 kHz
```

subject to safe Nyquist clamping.

Suggested default:

```text
~2–3 kHz
```

Mid 1 and Mid 2 must be completely independent.

They may:

- overlap
- cross over each other
- occupy the same region
- be used broadly
- be used surgically

There is no artificial low-mid/high-mid split.

## High Shelf

The High Shelf should be deliberately constrained to the upper-frequency region for broad brightness/air shaping.

Recommended continuous range:

```text
approximately 2 kHz – 20 kHz
```

subject to safe Nyquist clamping.

Suggested default:

```text
~8–10 kHz
```

The goal is a familiar mastering/bus-EQ workflow:

```text
Low Shelf  -> broad weight / warmth
Mid 1      -> broad or corrective bell
Mid 2      -> broad or corrective bell
High Shelf -> broad presence / air / sheen
```

---

All four bands should use:

```text
Gain
```

not "Cut".

Gain must support both boost and cut.

Recommended range:

```text
-18 dB to +18 dB
```

Default:

```text
0 dB
```

At 0 dB, each band should be effectively neutral.

---

## Mid 1 / Mid 2

Keep the current broad-to-surgical Q behaviour.

The current v3 mapping is approximately:

```text
Q 0.25 – 12
```

with logarithmic control mapping.

This is useful because it gives:

```text
~0.25–0.7   very broad / smooth shaping
~0.7–1.5    general musical EQ
~2–4        focused correction
~5–12       narrow / surgical work
```

Both mids should retain the ability to be broad or precise.

## Low Shelf / High Shelf

Do **not** blindly expose the full bell-Q range up to 12 on the shelves.

Shelf Q changes the contour around the turnover frequency and can create strong overshoot/resonance.

Start with a musically useful range such as:

```text
Q 0.5 – 2.0
```

with approximately:

```text
Q 0.707
```

as the neutral/default reference.

This should be listening-tested.

The goal is:

- broad and smooth when needed
- some controllable contour/character when desired
- no absurdly resonant master shelves

If testing shows a smaller range sounds better, prefer musical usefulness over theoretical range.

---

The NT should show real musical values rather than only abstract internal integers.

Use `parameterString()` where necessary.

Display examples:

```text
Low Shelf Freq: 120 Hz
Low Shelf Gain: +3.0 dB
Low Shelf Q:    0.71

Mid 1 Freq:     420 Hz
Mid 1 Gain:     -2.5 dB
Mid 1 Q:        0.80

Mid 2 Freq:     3.20 kHz
Mid 2 Gain:     -4.0 dB
Mid 2 Q:        5.00

High Shelf Freq: 8.00 kHz
High Shelf Gain: +2.0 dB
High Shelf Q:    0.71
```

This is important for listening tests and tuning the final ranges.

---

Keep the same stereo architecture already used by the current EQ.

Each band should share coefficients between left and right, but maintain **independent filter state** for each side.

Conceptually:

```text
LEFT:
Low Shelf -> Mid 1 -> Mid 2 -> High Shelf

RIGHT:
Low Shelf -> Mid 1 -> Mid 2 -> High Shelf
```

Do not sum stereo audio to mono.

Do not share biquad delay/history state between L/R.

---

Preserve the existing EQ parameter smoothing.

Frequency, Gain, and Q should continue to move smoothly.

The EQ should remain comfortable to adjust live without:

- zipper noise
- clicks
- unstable coefficient jumps
- NaNs/infinities

The `EQ Enable` switch should also transition cleanly.

Do not redesign the smoothing system unless there is an actual problem with the current implementation.

---

Do not alter the existing Filter DSP.

The Filter is already correctly stereo and has separate L/R state.

The EQ change is isolated to the EQ stage.

Preserve the current master processing order unless there is a deliberate, separately approved reason to change it.

The current intended relationship remains:

```text
... master processing ...
-> Filter
-> EQ
-> remaining master/output path
```

Do not use this EQ work as an excuse to restructure unrelated routing.

---

The current master EQ uses:

```text
3 stereo biquad stages
```

The new EQ uses:

```text
4 stereo biquad stages
```

This adds only one additional biquad per side.

Persistent EQ state remains tiny.

The more important resource concern on the NT is code/object memory and the existing overall Witchboard runtime memory footprint.

Therefore after implementation inspect:

- `.text`
- `.data`
- `.data.rel.ro`
- `.rodata`
- `.bss`
- `calculateRequirements()` memory requests
- unresolved symbols

Do not import the full Signalsmith library.

Continue using a small local adaptation of only the required coefficient-design maths.

---

## EQ tests

At minimum add/update tests for:

## EQ bypass

With:

```text
EQ Enable = Off
```

the EQ must be neutral.

## Flat EQ

With all gains:

```text
0 dB
```

the EQ must be effectively unity.

## Low Shelf

Verify:

- positive gain raises frequencies below the shelf region
- negative gain lowers them
- response approaches unity above the transition

## High Shelf

Verify:

- positive gain raises frequencies above the shelf region
- negative gain lowers them
- response approaches unity below the transition

## Mid 1 / Mid 2

Verify both:

- sweep independently across full range
- can cross each other
- can use different Q values
- can boost or cut independently

## Surgical Q

Verify that a high-Q mid cut is narrow and stable.

## Broad Q

Verify that a low-Q mid move is smooth and broad.

## Stereo independence

Verify no L/R state leakage.

## Sample rates

Test at least:

```text
32 kHz
48 kHz
96 kHz
```

and verify safe high-frequency behaviour.

## Live control

Move all controls while processing audio and verify:

- no NaNs
- no infinities
- no unstable bursts
- no obvious zippering/clicks

---

## EQ hardware acceptance

After implementation:

1. Build host tests.
2. Build the NT object.
3. Run object inspection.
4. Check unresolved symbols.
5. Inspect `._nt_dram` placement and confirm only intended cold functions moved.
6. Confirm **10-channel Witchboard loads reliably**.
7. Confirm 11 channels are no longer exposed.
8. Test `EQ Enable`.
9. Test Low Shelf.
10. Test Mid 1.
11. Test Mid 2.
12. Test High Shelf.
13. Test overlapping/crossed Mid bands across the usable spectrum.
14. Test broad smile-style master shaping.
15. Test narrow corrective cuts.
16. Test the EQ together with the existing Filter.
17. Confirm stereo imaging remains intact.
---

## Phase 3 acceptance gate

Commit only after:

- 4-band EQ is stable
- EQ Enable works
- shelves and both mids sound correct
- mids can overlap/cross
- stereo behaviour is correct
- sample-rate tests pass
- live control is stable
- total public parameter count is exactly the expected value
# PHASE 4 — Convert and validate Dynamics3 in isolation

**Goal:** get a faithful Airwindows Dynamics3 ARM/NT conversion working
and measured **before** wiring it into Witchboard.

This is intentionally separate from the shared-editor/preset architecture.
The point is to prove the DSP conversion, object size, dependencies and sound
first.

## Dynamics3 target and why it was chosen

The agreed target for Witchboard's future per-channel compressor is now:

**Airwindows Dynamics3 — Chris Johnson / Airwindows**

Primary references:

- Dynamics3 release/demo and design notes:  
  https://www.airwindows.com/dynamics3/
- Airwindows source repository:  
  https://github.com/airwindows/airwindows
- Airwindows license: MIT

Dynamics3 was chosen after listening to Chris Johnson's own demonstration and reviewing his explanation of the design.

This is **not** a request for a generic textbook compressor.

The goal is to preserve the musical, playable character of Dynamics3 as faithfully as practical on the disting NT.

## Why Dynamics3 was chosen for Witchboard

Dynamics3 fits Witchboard unusually well because it is:

- highly musical rather than clinical
- designed to be played by ear
- capable of subtle compression and very obvious character
- based on Chris Johnson's newer Bezier-spline dynamics work
- able to move continuously from compression through dry to complementary expansion
- zero-latency
- nonlinear and characterful without being a component-level hardware emulation
- controlled by a very small number of meaningful parameters
- open source under the MIT licence

Chris describes the core workflow as essentially:

```text
Threshold
Attack
Release
Inv/Wet
```

That compact control set is ideal for the NT's strict parameter budget.

Do not replace Dynamics3's behaviour with a conventional:

```text
Threshold / Ratio / Attack / Release / Makeup
```

compressor unless hardware constraints make the faithful design impossible.

The distinctive Dynamics3 behaviour is the reason it was selected.

---

## Dynamics3 character that must be preserved

Dynamics3 uses Chris Johnson's Bezier-spline dynamics approach.

The implementation should preserve, as far as practical:

- the spline-based gain behaviour
- the original threshold response
- the original attack behaviour
- the original release behaviour
- the original Inv/Wet compression-to-expansion behaviour
- any internal makeup/level compensation that is part of the original algorithm
- any protective clipping/safety behaviour that is genuinely part of Dynamics3
- the nonlinear character which appears at harder settings

Do not "clean up" audible behaviour merely because it differs from a conventional compressor.

Chris explicitly demonstrates that extreme attack/release/threshold combinations can create:

- warble
- bloom
- twang
- very aggressive compression
- limiter-like behaviour
- expansion which reshapes the body after the transient

Those behaviours are part of why Dynamics3 was chosen.

---

## Stereo behaviour to verify from source

Witchboard channels are stereo.

The Dynamics3 adaptation must make an intentional decision about stereo detector behaviour after inspecting the actual current Dynamics3 source.

Preferred Witchboard behaviour is:

```text
stereo-linked gain control
```

so a transient on one side does not pull the stereo image around unpredictably.

However, fidelity to the actual Dynamics3 algorithm matters.

Before implementation:

1. inspect the current Dynamics3 source
2. document how its stereo version derives gain/control state
3. decide whether a linked adaptation is necessary
4. listening-test both fidelity and stereo-image stability if the original is unlinked

Do not silently alter the algorithm without documenting the change.

---

## NT suitability / object inspection

The current `Dynamics.o` / `DynamicsMono.o` objects are useful only as Airwindows ARM baselines.

They are **not** substitutes for Dynamics3.

Dynamics3 itself still needs an NT-compatible conversion/build before final implementation.

Once available, inspect:

- `.text`
- `.data`
- `.data.rel.ro`
- `.rodata`
- `.bss`
- unresolved symbols
- per-instance persistent state
- sample-rate assumptions
- expensive maths in the audio loop
- any undersampling behaviour
- any random/dither dependencies
- branch behaviour when bypassed

Then benchmark:

```text
1 enabled
2 enabled
4 enabled
10 enabled
```

with representative Witchboard routing and EQ/filter use.

The goal is fidelity first, followed by targeted optimisation which does not change the sound.

---

## Phase 4 acceptance gate

Before Witchboard integration:

1. Build the converted Dynamics3 object for the NT toolchain.
2. Inspect code/data/state sizes and unresolved symbols.
3. Confirm sample-rate handling.
4. Confirm bypass can be made cheap.
5. Listen to it independently and verify it still sounds like Dynamics3.
6. Measure one-instance CPU cost.
7. Document any unavoidable deviations from Chris Johnson's original algorithm.

Do not proceed merely because it compiles; the sound is the reason this
compressor was chosen.
# PHASE 5 — Integrate Dynamics3 as per-channel dynamics

**Goal:** add ten independent Dynamics3 settings/state sets to Witchboard
using one shared public editor.

Target after this phase:

```text
237 total parameters
```

The shared controls are global editor controls; the stored compressor settings
are independent per channel.

## Shared control model

The shared Witchboard editor should expose:

```text
Comp Channel
Comp Enable
Threshold
Attack
Release
Inv/Wet
```

Total public parameters:

```text
6
```

`Comp Channel` selects which channel's stored compressor settings are being edited.

The settings themselves are **not global**.

Each of the 10 Witchboard channels must retain its own independent:

```text
Enable
Threshold
Attack
Release
Inv/Wet
```

values.

Conceptually:

```cpp
struct Dynamics3Settings
{
    bool enabled;
    // internal/native representations chosen during implementation
    float threshold;
    float attack;
    float release;
    float invWet;
};

Dynamics3Settings dynamics[kMaxChannels];
```

The public controls are only a shared editor window into those ten independent settings.

---

## Inv/Wet behaviour

`Inv/Wet` is a defining part of Dynamics3 and must not be renamed or simplified into an ordinary `Mix` control without a very good reason.

Its conceptual behaviour is:

```text
Wet side      -> Dynamics3 compression
Centre        -> dry / effectively neutral
Inv side      -> complementary expansion
```

The expansion is not a gate.

It uses the same underlying dynamics timing concept in the opposite direction, allowing behaviour such as:

- suppressing the initial transient and blooming the body behind it
- widening or softening percussive attacks
- de-ambience style behaviour
- dense compression
- subtle bus-like compression
- extreme sound-design movement

Preserving this compression/dry/expansion continuum is more important than making the UI look like a conventional compressor.

---

## Parameter budget

After the agreed 10-channel + 4-band master EQ:

```text
231 parameters
```

Confirmed disting NT hard maximum:

```text
241 parameters
```

Dynamics3 shared editor:

```text
Comp Channel = 1
Comp Enable  = 1
Threshold    = 1
Attack       = 1
Release      = 1
Inv/Wet      = 1
----------------
Total        = 6
```

Projected final count:

```text
231 + 6 = 237 parameters
```

Remaining headroom:

```text
241 - 237 = 4 parameters
```

This is valid.

Maintain:

```cpp
static_assert(kMaxParams <= 241,
              "disting NT supports at most 241 parameters per algorithm");
```

If Expert Sleepers later exposes a named constant for this limit, use that instead of hard-coding `241`.

Do not spend the remaining four parameters casually.

---

## Bypass / CPU strategy

Each channel needs its own compressor enable state.

When a channel compressor is disabled, bypass the Dynamics3 processing as early and cheaply as practical.

Conceptually:

```cpp
if (!settings.enabled)
{
    // pass audio unchanged
}
else
{
    // Dynamics3 processing
}
```

The user does not expect all ten compressors to be enabled simultaneously during normal use, so disabled channels should incur negligible runtime DSP cost.

However, the implementation must still be stress-tested with:

```text
all 10 compressors enabled
```

to ensure the plugin remains safe on the disting NT.

Do not assume average use is sufficient for hardware validation.

---

## Internal state / preset storage

Because only six NT parameters are used as the shared editor, the ten channels' actual Dynamics3 settings must be stored internally by Witchboard.

Requirements:

- each channel retains independent settings
- changing `Comp Channel` refreshes the editor values from that channel
- changing an editor control updates only the selected channel
- switching channels must not alter another channel's compressor
- all ten channels' settings must survive preset save/load
- enable state must also survive preset save/load

This means the custom Witchboard serialisation needs to store the per-channel dynamics settings.

Do not attempt to expose all per-channel settings as NT parameters; that would exceed the 241-parameter ceiling.

## MIDI mapping caveat

Normal NT MIDI mappings belong to public NT parameters.

Because the shared controls edit whichever `Comp Channel` is selected, MIDI mapping of these controls will naturally address the **currently selected compressor channel**, not ten permanently distinct public parameters.

Document this behaviour clearly once implemented.

Do not invent hidden NT parameters to work around it.

---

## Compressor placement

Do not guess the final insertion point.

Before coding, document the exact current per-channel signal order and choose the Dynamics3 location intentionally.

Likely goal:

```text
input/channel gain
-> Dynamics3
-> channel inserts / sends / routing
```

or another position which best behaves like a useful channel-strip compressor.

The exact location should be listening-tested because it changes how external inserts and FX sends react to compression.

Do not alter unrelated routing while adding it.

---

## Dynamics3 acceptance tests

At minimum:

## Independence

Set very different compressor settings on channels 1, 2 and 3.

Switch `Comp Channel` repeatedly.

Confirm each channel recalls only its own settings.

## Enable

Confirm every channel has an independent enable state.

Disabled must be neutral.

## Compression

Test:

- gentle threshold settings
- slow attack
- fast attack
- slow release
- fast release
- extreme compression

Verify the response remains recognisably Dynamics3-like.

## Inv/Wet

Verify:

```text
compression side
centre/dry
expansion side
```

and confirm smooth, useful movement across the entire control.

## Character

Listening-test drums, bass, synths and full-range material.

The goal is not merely numerical correctness.

The result should retain the musical movement and character that motivated choosing Dynamics3.

## Presets

Save a preset with obviously different Dynamics3 settings on several channels.

Load another preset.

Reload the test preset.

Confirm every Dynamics3 setting and enable state restores correctly.

## Worst-case CPU

Enable all ten compressors together with the 4-band EQ and existing filter available.

Confirm:

- no audio dropouts
- no runaway CPU
- no NaNs/infinities
- no instability
- no memory failure

---

## Phase 5 acceptance gate

Do not call this complete until:

- each channel retains its own Enable/Threshold/Attack/Release/Inv-Wet values
- switching `Comp Channel` only changes the editor view
- preset save/load restores all 10 channels correctly
- MIDI mapping behaviour is documented
- disabled channels take negligible DSP time
- all 10 enabled is stress-tested
- final parameter count remains <= 241
- filter + EQ + Dynamics3 together are stable on hardware
# PHASE 6 — Final profiling, cleanup and release documentation

**Goal:** optimise only after all features are individually proven.

Do not combine speculative optimisation with the feature implementation
commits.

## Final profiling

Re-measure:

- `.text`
- `._nt_dram`
- `.data`
- `.data.rel.ro`
- `.rodata`
- `.bss`
- per-instance SRAM/DRAM/DTC/ITC requests
- CPU with realistic use
- CPU with all ten Dynamics3 instances enabled

Only after measuring should additional cold functions be considered for
`_NT_DRAM_SECTION`.

Keep hot DSP in fast code memory unless profiling gives a compelling reason to
do otherwise.

## Documentation

After hardware validation, update:

```text
README.md
CHANGELOG.md
```

README should describe the final user-facing EQ simply as:

```text
4-band master EQ:
Low Shelf / Mid 1 / Mid 2 / High Shelf
```

with:

```text
Freq / Gain / Q
```

for each band plus:

```text
EQ Enable
```

Both mids should be documented as wide-ranging, fully independent bell bands that can overlap and cross freely.

Keep the Signalsmith credit and MIT attribution.

Do not add unnecessary extra permanent documentation files.

---

## README DSP credits / rationale

The README should place the three key DSP credits **near the top**, before the feature list, with direct links and a short explanation of why each was chosen specifically for Witchboard.

Use wording substantially like:

### Filter — Matthijs Hollemans / Cytomic SVF

Source:
https://gist.github.com/hollance/2891d89c57adc71d9560bcf0e1e55c4b

Underlying design:
Andrew Simper / Cytomic, *SvfLinearTrapOptimised2*
http://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf

Reason:
chosen because its topology-preserving state-variable design gives Witchboard the smooth, immediate and musical DJ-style HP/LP sweep behaviour the project wanted, while remaining compact and practical for the disting NT.

### Master EQ — Signalsmith Audio / Geraint Luff

Source:
https://github.com/Signalsmith-Audio/dsp

Filter reference:
https://github.com/Signalsmith-Audio/dsp/blob/main/filters.h

Reason:
chosen for a high-quality, well-tested and compact C++ biquad design with the exact peaking and shelf responses Witchboard needs, including sensible behaviour close to Nyquist, without pulling a heavyweight DSP framework into the NT plugin.

### Per-channel Dynamics — Airwindows Dynamics3 / Chris Johnson

Release/demo:
https://www.airwindows.com/dynamics3/

Source:
https://github.com/airwindows/airwindows

Reason:
chosen because its Bezier-spline dynamics and unique Inv/Wet compression-to-expansion behaviour sound unusually musical and playable, giving each Witchboard channel a characterful dynamics instrument rather than a generic utility compressor, while its four core sound controls fit the NT's severe parameter budget extremely well.

Make clear in the README until implementation is complete that Dynamics3 is the **selected/planned per-channel dynamics design**, not yet a shipped feature.

---

## Final architecture

Current agreed direction:

```text
Supported channels:                10

Current 10-channel parameters:     227

4-band master EQ:
Low Shelf
Mid 1 Bell
Mid 2 Bell
High Shelf
EQ Enable

Projected after EQ:                231

Per-channel Dynamics target:
Airwindows Dynamics3

Shared dynamics editor:
Comp Channel
Comp Enable
Threshold
Attack
Release
Inv/Wet

Projected dynamics editor cost:    +6

Projected total:                   237

Confirmed NT maximum:              241

Remaining headroom:                4
```

The 10-channel cap is intentional.

Do not attempt to restore 11 channels by deleting controls unless the product direction changes explicitly.

The filter should remain faithful to the Matthijs Hollemans / Cytomic SVF source.

The EQ should remain a compact Signalsmith-derived implementation.

The compressor target is now Airwindows Dynamics3, preserved as faithfully as the NT platform allows.

## Final release gate

The final release candidate should have one known-good commit after each phase:

```text
1. DRAM placement
2. 10-channel cap
3. 4-band EQ
4. Dynamics3 conversion
5. Dynamics3 integration
6. final profiling/docs/release
```

This sequence is intentional. If a later phase introduces a problem, return to
the immediately previous known-good commit rather than debugging several
unrelated changes at once.
