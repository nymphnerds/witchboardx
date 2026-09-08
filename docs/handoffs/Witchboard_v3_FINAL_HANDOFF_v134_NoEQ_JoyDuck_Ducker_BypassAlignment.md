# Witchboard v3 — Final Implementation Handoff v1.34
## No EQ + JoyDuck-derived trigger ducker + Bypass latency alignment

This document is a **clean final-check rewrite** of the previous cumulative handoffs.
It supersedes v1.33 and removes stale/conflicting wording.

Current source checked:

```text
Witchboard-master.zip
plugins/Witchboard/Witchboard.cpp
```

Reference device checked:

```text
JoyDuck 1.7.amxd
```

---

# 1. Locked product direction

## Keep

```text
10-channel current source as implementation baseline
current direct per-channel parameter identities
current inserts/routing
Master Filter
Final Master Gain
Master modes: Split / Sum / Insert
trigger-driven master ducking
```

## Remove

```text
built-in 3-band master EQ (9 params)
old SC Key Mode
old SC Attack
old SC Hold
old SC Release
old SC Makeup
old x^9-style SC curve code
gate/audio sidechain modes
```

## New ducking controls

```text
Sidechain
SC Trigger Input
SC Depth
SC Lookahead
SC Env Length
SC Curve
SC Smooth
```

## New routing/latency control

```text
Bypass Offset
```

Total new sidechain/latency control set:

```text
8 globals:
7 ducker params
+ 1 Bypass Offset
```

The ducker is an **inverse-envelope VCA**, not a compressor.

---

# 2. Current-source facts verified

Current source assertions:

```cpp
static_assert(kMaxParams == 229, "10-channel parameter budget changed");
static_assert(kNumGlobalParams == 79, "global parameter count changed");
static_assert(kNumChannelParams == 15, "channel parameter count changed");
static_assert(kMaxParams <= 241,
              "disting NT supports at most 241 parameters per algorithm");
```

Current sidechain = 9 params:

```text
Sidechain
SC key input
SC key mode
SC depth
SC attack
SC release
SC curve
SC makeup
SC hold
```

Current EQ = 9 params:

```text
EQ1 Freq / Gain / Q
EQ2 Freq / Gain / Q
EQ3 Freq / Gain / Q
```

Current per-channel count remains:

```text
15 params/channel
```

Do not refactor the channel UI/mapping architecture in this pass.

---

# 3. Final parameter budget

Starting point:

```text
79 globals
15 × 10 channels = 150
total = 229
```

Remove 9 EQ params:

```text
79 -> 70 globals
```

Replace old 9-param sidechain with new 7-param ducker:

```text
70 -> 68 globals
```

Add visible `Bypass Offset`:

```text
68 -> 69 globals
```

Final design budget:

```text
10 channels:
69 + 150 = 219 / 241
22 spare

11 channels:
69 + 165 = 234 / 241
7 spare

12 channels:
69 + 180 = 249 / 241
8 over
```

Therefore:

> **11 channels fit with the current direct/mappable channel architecture.**

> **12 channels still require 8 additional parameter savings.**

Do not force a shared-editor refactor merely to reach 12 channels.

---

# 4. Final ducker signal model

Trigger path:

```text
SC Trigger Input
    ↓
rising-edge detection
    ↓
raw normalized envelope resets to 0
    ↓
curved 0 -> 1 recovery
    ↓
SC Smooth
    ↓
SC Depth
    ↓
VCA gain
```

Program path:

```text
Main submix
    ↓
SC Lookahead delay
    ↓
VCA
    ↓
master routing / processing
```

Meaning:

```text
raw envelope 0 = deepest duck
raw envelope 1 = unity
```

There is:

```text
NO user Attack
NO Hold
NO Release
NO threshold
NO key HPF
NO Gate mode
NO Audio detector mode
NO Makeup
NO breakpoint editor
```

---

# 5. Trigger behavior

Witchboard uses a clean trigger input only.

Use the existing source convention initially:

```cpp
const bool keyHigh = fabsf(keySample) > 0.1f;
const bool trigger = keyHigh && !previousKeyHigh;
```

On a rising trigger:

```text
start a new duck envelope
raw envelope begins at 0
```

No separate public retrigger feature exists.

A later trigger simply fires the same envelope again as normal trigger behavior.

---

# 6. No breakpoints — important

JoyDuck uses Max `function` because JoyDuck is a general-purpose editable envelope shaper.

Witchboard does **not** use that architecture.

There is:

```text
no breakpoint table
no point list
no function editor
no breakpoint interpolation
```

Witchboard has exactly one normalized trajectory:

```text
x = 0 -> full duck
x = 1 -> unity
```

`SC Curve` defines the path between those endpoints.

`SC Env Length` defines how long the complete 0 -> 1 trajectory lasts.

---

# 7. SC Env Length — LOCKED

Public range:

```text
50 .. 2000 ms
```

Use true logarithmic parameter mapping:

```cpp
float envLengthFromNormalized(float x)
{
    x = clampFloat(x, 0.0f, 1.0f);

    const float minMs = 50.0f;
    const float maxMs = 2000.0f;

    return minMs * powf(maxMs / minMs, x);
}
```

Representative positions:

```text
0%      50 ms
25%    126 ms
50%    316 ms
75%    796 ms
100%  2000 ms
```

Initial audition default:

```text
~300 ms
```

Convert to samples:

```cpp
lengthSamples =
    max(1,
        roundToInt(
            envLengthMs * sampleRate * 0.001f));
```

This is the Witchboard simplification of JoyDuck's whole-envelope time scaling.

---

# 8. SC Curve — preferred implementation

Public semantics:

```text
negative = fast initial recovery, soft tail
zero     = linear
positive = slow initial recovery, steeper finish
```

Recommended public range:

```text
-100 .. +100
```

Map to:

```cpp
c = parameter * 0.01f;   // -1 .. +1
```

The current Witchboard `x^9` blend must be removed.

## Curve coefficient mapping

Use the established Max-community curve mapping:

```cpp
float curveToBeta(float c)
{
    c = clampFloat(c, -1.0f, 1.0f);

    if (fabsf(c) < 0.001f)
        return 0.0f;

    float hp =
        powf((fabsf(c) + 1.0e-20f) * 1.2f, 0.41f) * 0.91f;

    hp = clampFloat(hp, 0.0f, 0.999999f);

    const float fp = hp / (1.0f - hp);

    return c < 0.0f ? -fp : fp;
}
```

## Per-envelope recurrence

For `N = lengthSamples`:

```cpp
beta = curveToBeta(curve);
```

Linear special case:

```cpp
increment = 1.0f / N;
multiplier = 1.0f;
```

Curved case:

```cpp
q = expf(beta / N);

increment =
    (q - 1.0f)
    / (expf(beta) - 1.0f);

multiplier = q;
```

Per sample:

```cpp
value += increment;
increment *= multiplier;
```

Clamp/force the final sample to exactly:

```text
1.0
```

This recurrence has the closed form:

```text
(exp(beta*x) - 1) / (exp(beta) - 1)
```

with:

```text
x = n/N
```

and avoids a per-sample `expf()`.

## Curve fidelity caveat

The recurrence structure is strongly grounded in public Cycling '74 discussion of the Gen `curve~` emulation.

The exact original `gen~.curve.maxpat` operator graph has still not been recovered.

Therefore:

```text
do not claim byte-for-byte Max curve~ emulation
```

but this is the preferred implementation candidate.

References:

```text
https://docs.cycling74.com/reference/curve~/
https://cycling74.com/forums/math-behind-the-curve-object
https://cycling74.com/forums/math-behind-function-curve
```

---

# 9. SC Smooth — LOCKED from JoyDuck

JoyDuck patch path:

```text
Smooth
-> * 2
-> milliseconds
-> mstosamps~
-> rampsmooth~
```

JoyDuck comment:

```text
100% = 200 ms
```

Therefore:

```cpp
smoothMs = smoothValue * 2.0f;
```

Public range:

```text
0 .. 100
```

Reference values:

```text
0   =   0 ms
1   =   2 ms
2   =   4 ms
4   =   8 ms
10  =  20 ms
50  = 100 ms
100 = 200 ms
```

The creator's recommended/video reference:

```text
Smooth = 4
```

therefore:

```text
8 ms
```

Use the same sample count for rise and fall.

Cycling '74 documents `rampsmooth~` as starting a linear ramp to each new incoming value over the chosen number of samples.

Reference:

```text
https://docs.cycling74.com/reference/rampsmooth~/
```

## Processing order

LOCK:

```text
Curve
-> Smooth
-> Depth
-> VCA
```

Do **not** smooth after Depth.

JoyDuck also has a tiny fixed `rampsmooth~ 10 10` later in the chain.
Treat that as optional internal cleanup only; do not expose it.

---

# 10. SC Depth — LOCKED

Use a simple linear VCA law:

```cpp
depth =
    clampFloat(depthPercent * 0.01f, 0.0f, 1.0f);

vcaGain =
    1.0f - depth * (1.0f - smoothedEnvelope);
```

Equivalent:

```cpp
vcaGain =
    (1.0f - depth) + depth * smoothedEnvelope;
```

Properties:

```text
Depth   0% -> unity always
Depth  50% -> deepest gain 0.5 ≈ -6.02 dB
Depth 100% -> deepest gain 0.0
Envelope 1 -> unity
```

Do not introduce a dB-depth law.

---

# 11. SC Lookahead — LOCKED architecture

Public range:

```text
0 .. 10 ms
```

Initial reference/audition value:

```text
~6 ms
```

JoyDuck uses the standard conversion:

```cpp
lookaheadSamples =
    roundToInt(
        lookaheadMs * sampleRate * 0.001f);
```

Examples:

```text
48 kHz / 6 ms  = 288 samples
96 kHz / 10 ms = 960 samples
```

Trigger/envelope path remains immediate.

Only the Main audio branch is delayed for lookahead.

This is what lets `Smooth` soften the initial duck before the transient reaches the VCA.

Suggested first JoyDuck-like pairing:

```text
SC Lookahead ≈ 6 ms
SC Smooth     = 4  (= 8 ms)
```

Do not hard-code that relationship; they remain independent controls.

---

# 12. Current routing — verified from source

Current source builds two submixes every frame:

```text
mainLeft / mainRight
bypassLeft / bypassRight
```

Per-channel `Output Path` selects which submix receives the channel.

FX returns may also be routed to Main or Bypass.

Current master behavior:

## Split

```text
Main submix
-> sidechain
-> master processing
-> Main L/R

Bypass submix
-> Bypass L/R
```

## Sum

```text
Main submix
-> sidechain
     \
      + -> sum -> master processing -> Main L/R
     /
Bypass submix
```

## Insert

```text
Main submix
-> sidechain
     \
      + -> sum -> master processing
     /
Bypass submix
-> Master Send
-> Master Return
-> Main L/R
```

This routing is important for `Bypass Offset`.

---

# 13. Bypass Offset — FINAL semantics

Add one visible global parameter:

```text
Bypass Offset
```

This is the **effective delay applied to the Bypass submix**.

It is useful in every master mode:

```text
Split:
    delays physical Bypass L/R

Sum:
    delays Bypass submix BEFORE it recombines with Main

Insert:
    delays Bypass submix BEFORE it recombines with Main and enters
    the master insert
```

This corrects the stale earlier wording that treated it as Split-only.

## Automatic SC compensation

When the sidechain is enabled:

```text
Main branch gets SC Lookahead delay
```

therefore the Bypass branch should automatically receive the same baseline delay.

The user wanted this automatic contribution to be **visible in the Bypass Offset parameter**.

Define:

```text
effectiveBypassMs = autoLookaheadContribution + manualTrim
```

where:

```text
autoLookaheadContribution =
    sidechainEnabled ? scLookaheadMs : 0
```

Physical delay is:

```cpp
max(0, effectiveBypassMs)
```

## Example

```text
SC enabled
SC Lookahead = 6 ms
manual trim  = +4 ms
```

display:

```text
Bypass Offset = 10 ms
```

Change Lookahead to 8 ms:

```text
manual trim remains +4 ms
Bypass Offset automatically becomes 12 ms
```

## Direct user edit

If:

```text
SC Lookahead = 6 ms
Bypass Offset currently = 10 ms
```

and user turns Bypass Offset to:

```text
13 ms
```

then:

```text
manual trim = +7 ms
```

If Lookahead later becomes 8 ms:

```text
Bypass Offset = 15 ms
```

## SC enable/disable

The automatic part must follow actual active lookahead.

Therefore:

```text
SC OFF:
auto contribution = 0
manual trim remains

SC ON:
auto contribution = current SC Lookahead
manual trim remains
```

This prevents the Main branch from being undelayed while Bypass still contains stale automatic SC compensation.

---

# 14. Why Bypass Offset matters in Sum mode

This is especially relevant to the user's normal Sum-mode workflow.

Correct Sum topology:

```text
Main channels
-> optional per-channel/external inserts
-> SC Lookahead delay
-> duck VCA
----------------------------\
                              + -> master processing -> Main out
Bypass channels               /
-> Bypass Offset delay
----------------------------/
```

Example:

```text
SC Lookahead = 6 ms
Bypass Offset = 6 ms
```

The two branches line up at the sum.

If the Main branch has an additional external/iPad path that makes it
another 7 ms late, manually set:

```text
Bypass Offset = 13 ms
```

to line the Bypass branch up with it.

This is precisely why the visible manual control is useful.

Important limitation:

> If the Bypass branch itself is the later branch, a positive-only physical
> Bypass delay cannot move it earlier than zero.

That case belongs to the future true signed Witchboard latency system.

---

# 15. Bypass Offset range

First implementation:

```text
0 .. 100 ms
```

Reason:

```text
0..10 ms   -> direct SC Lookahead compensation
10..100 ms -> practical external/iPad/hardware latency alignment
```

This is still a latency-control range, not a creative delay effect.

If the NT parameter framework cleanly supports finer-than-1-ms control,
fractional-ms display/control would be useful.

Do not assume fractional resolution until verified in the API/UI.

---

# 16. Bypass Offset auto-follow implementation rule

The **displayed/stored public parameter should represent the effective delay**.

Avoid making the visible number mean one thing while MIDI/CV mapping means another.

Recommended logic:

```text
on live SC Lookahead change:
    delta = newLookahead - oldLookahead
    if SC enabled:
        Bypass Offset += delta

on SC enable:
    Bypass Offset += currentLookahead

on SC disable:
    Bypass Offset -= currentLookahead
```

Clamp to valid range.

This naturally preserves the manual trim because:

```text
manualTrim =
    BypassOffset - activeAutoContribution
```

## Presets / deserialisation

Do NOT accidentally double-apply the automatic adjustment while loading a preset.

During deserialisation:

```text
- restore saved parameter values
- suppress live auto-follow side effects
- establish internal previous-lookahead / enable state
- only resume auto-follow after load completes
```

This needs an explicit test.

Also test MIDI/CV-mapped Lookahead changes because they must update the effective Bypass Offset exactly like front-panel changes.

---

# 17. Delay DSP placement by master mode

## Main Lookahead delay

Always applied to the **Main submix before the duck VCA** when SC is enabled.

```text
Main submix
-> Main lookahead circular buffer
-> VCA
```

## Bypass Offset delay

Always applied to the **Bypass submix before its mode-dependent destination**.

```text
Bypass submix
-> Bypass Offset circular buffer
```

Then:

```text
Split:
    -> physical Bypass L/R

Sum:
    -> sum with processed Main

Insert:
    -> sum with processed Main
    -> master insert
```

This is the cleanest mode-independent implementation.

---

# 18. Delay memory

SC Lookahead maximum:

```text
10 ms @ 96 kHz
= 960 samples/channel
= 1920 floats stereo
≈ 7.5 KiB
```

Bypass Offset maximum:

```text
100 ms @ 96 kHz
= 9600 samples/channel
= 19200 floats stereo
≈ 75 KiB
```

Combined worst-case stereo storage if implemented as independent float rings:

```text
~82.5 KiB
```

This is not tiny.

Allocate intentionally in appropriate per-instance DRAM, not stack/DTC.

Do not allocate 100 ms per channel; `Bypass Offset` operates on the already-summed stereo Bypass branch.

Future per-channel signed compensation will require a separate memory-budget review.

---

# 19. Changing delay values while audio is running

A hard read-pointer jump can click.

Both:

```text
SC Lookahead
Bypass Offset
```

must change safely.

Preferred methods:

```text
crossfade old/new delay taps
or
smoothly move between delay taps with an artifact-safe strategy
```

Do not simply jump the circular-buffer read index during live audio.

Because these controls may be MIDI/CV mapped, live modulation/change handling is required.

---

# 20. Master Insert caveat

In current `Master output = Insert` mode, Main and Bypass branches recombine **before** the Master Send.

Therefore any latency added by the **master insert itself** affects the already-combined signal equally.

`Bypass Offset` is useful there for latency differences that exist **before the branches recombine**, for example per-channel/external processing on one branch.

It is not needed to compensate the Master Insert round trip against itself.

---

# 21. Sidechain disabled transparency

When:

```text
Sidechain = OFF
SC Lookahead auto contribution = 0
```

The Main path should not incur the SC Lookahead delay.

Manual Bypass Offset may still remain active because it has independent utility for external path alignment.

This gives:

```text
Sidechain OFF + Bypass Offset 0
-> no new latency from this system

Sidechain OFF + Bypass Offset >0
-> intentional Bypass-branch latency adjustment
```

---

# 22. Ordered implementation plan

## Phase 1 — Remove EQ

Delete:

```text
9 EQ parameters
EQ state/DSP
EQ tests/pages
obsolete preset migration specific to keeping that EQ
```

Expected 10-channel count:

```text
220
```

Build/test/commit.

## Phase 2 — Replace sidechain parameter model

New public params:

```text
Sidechain
SC Trigger Input
SC Depth
SC Lookahead
SC Env Length
SC Curve
SC Smooth
```

Expected count:

```text
218
```

Build/test parameter table before DSP changes.

## Phase 3 — Implement new envelope

Implement:

```text
rising trigger
normalized recurrence
Env Length
Curve
Smooth
Depth
```

No lookahead yet.

Verify envelope numerically.

Build/test/commit.

## Phase 4 — Implement SC Lookahead

Add stereo Main lookahead circular buffer.

Verify:

```text
0..10 ms
sample-rate correctness
safe live changes
trigger remains undelayed
```

Build/test/commit.

## Phase 5 — Add Bypass Offset

Add:

```text
Bypass Offset 0..100 ms
```

Expected final 10-channel count:

```text
219
```

Apply Bypass delay before Split/Sum/Insert destination.

Implement SC auto-follow semantics.

Build/test/commit.

## Phase 6 — hardware audition

Only after all automated tests pass.

---

# 23. Automated acceptance tests

Run at:

```text
32 kHz
44.1 kHz
48 kHz
96 kHz
```

## Envelope

```text
1. Curve 0 is linear.
2. Negative Curve recovers faster initially than linear.
3. Positive Curve recovers slower initially than linear.
4. Curve is monotonic.
5. Envelope lands exactly at 1 after Env Length.
6. No NaN/Inf at Curve extremes.
7. Env Length sample count is correct.
```

## Smooth

```text
8. Smooth 0 passes envelope unchanged.
9. Smooth 4 corresponds to 8 ms.
10. same smoothing time is used for rise/fall.
11. continuous curved input is handled without instability.
```

## Depth

```text
12. Depth 0 is unity.
13. Depth 50 reaches 0.5 at deepest point.
14. Depth 100 reaches 0 at deepest point.
15. unity endpoint remains unity at all Depth values.
```

## Lookahead

```text
16. Lookahead 0 adds zero SC latency.
17. Lookahead 6 ms gives correct sample delay.
18. Lookahead 10 ms works at 96 kHz.
19. trigger path is not delayed.
20. live Lookahead changes do not click.
```

## Bypass Offset

```text
21. Split: Bypass branch is delayed before physical Bypass output.
22. Sum: Bypass branch is delayed before recombination.
23. Insert: Bypass branch is delayed before recombination/master insert.
24. SC ON auto-adds current Lookahead to effective Bypass Offset.
25. SC OFF removes only the auto contribution.
26. manual trim is preserved across Lookahead changes.
27. direct Bypass Offset edits derive the correct manual trim.
28. live Bypass Offset changes do not click.
29. preset load does not double-apply auto compensation.
30. MIDI/CV-mapped Lookahead updates Bypass Offset correctly.
```

## Routing

```text
31. Main and Bypass FX-return routing still behaves correctly.
32. current channel Output Path behavior is unchanged.
33. Master Filter still processes exactly the same branches as current source.
34. Final Master Gain remains at the end of current master path.
```

---

# 24. Hardware audition reference settings

Start with:

```text
SC Depth      80
SC Lookahead   6 ms
SC Env Length 300 ms
SC Curve      -50
SC Smooth       4   (= 8 ms)
Bypass Offset   6 ms effective when SC is enabled and no manual trim
```

These are audition references, not sacred defaults.

Listen with:

```text
4/4 kick trigger
fast kick pattern
irregular trigger
bass line
pad
full mix
iPad insert path
Main vs Bypass recombination in Sum mode
```

Listen for:

```text
clicks
transient leakage
over-soft attack
pumping tail feel
phase/timing smear at recombination
delay-change artifacts
unexpected latency when SC is disabled
```

---

# 25. Future development — true signed Witchboard latency compensation

Separate future feature:

```text
per-channel Latency Offset
```

with signed requested offsets.

Only introduce a common mixer delay if a negative requested offset needs it:

```text
D_base = max(0, -min_i(offset_i))
D_i    = D_base + offset_i
```

This would allow channels to move earlier/later relative to the mixer while adding the minimum common latency.

Do not implement this in the ducker pass.

The current:

```text
SC Lookahead
Bypass Offset
```

use the same basic circular-delay primitive and can later integrate into that broader latency manager.

---

# 26. Final locked control list

Ducker:

```text
Sidechain
SC Trigger Input
SC Depth
SC Lookahead
SC Env Length
SC Curve
SC Smooth
```

Latency alignment:

```text
Bypass Offset
```

No other ducking controls.

---

# 27. Final check summary

Confirmed against current Witchboard source:

```text
- current count 229
- globals 79
- channels 15 params each
- EQ is 9 params
- old sidechain is 9 params
- Main/Bypass are separate internal submixes
- Sum recombines Bypass after sidechain
- Insert recombines Bypass after sidechain and before Master Send
- Split emits physical Bypass separately
- FX returns can route to Main or Bypass
```

Final planned count:

```text
219 / 241 at 10 channels
234 / 241 at 11 channels
249 / 241 at 12 channels
```

No stale assumption that `Bypass Offset` is Split-only remains.

No breakpoint implementation is planned.

No master EQ remains.

No Attack/Hold/Release/Makeup/Mode remains.

The main implementation risks still requiring testing are:

```text
1. exact Max curve~ coefficient fidelity
2. faithful rampsmooth~-style continuous smoothing
3. artifact-free live delay-time changes
4. preset/MIDI/CV behavior of auto-follow Bypass Offset
```

Everything else in this handoff is now internally consistent with the current source and the chosen design.
