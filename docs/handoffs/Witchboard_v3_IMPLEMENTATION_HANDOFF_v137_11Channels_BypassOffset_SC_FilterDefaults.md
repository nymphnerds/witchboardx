# Witchboard v3 — Implementation Handoff v1.35
## Promote to 11 channels + move Bypass Offset out of Sidechain UI

This handoff supersedes the v1.34 channel-count/UI placement assumptions.

---

# 1. Two changes only

## A. Promote supported channel count from 10 -> 11

The v1.34 parameter savings now make 11 channels fit cleanly.

Current final parameter structure:

```text
69 global parameters
15 parameters per channel
disting NT hard maximum = 241
```

Counts:

```text
10 channels = 69 + 150 = 219
11 channels = 69 + 165 = 234
12 channels = 69 + 180 = 249
```

Therefore:

```text
11 channels = SUPPORTED
12 channels = NOT supported with the current direct parameter architecture
```

11 channels leaves:

```text
241 - 234 = 7 spare parameters
```

The current source remaining capped at 10 is only a stale implementation cap
from using the old 10-channel build as the v1.34 baseline.

Change the supported/default maximum channel count to:

```text
11
```

Do not redesign the ducker or channel parameter model for this.

---

# 2. 11-channel implementation checklist

Update every place where the source assumes or asserts 10 channels.

Search for:

```text
10
kNumChannels
kMaxChannels
channel count specs
parameter-count static_asserts
array sizes tied to channel count
UI page generation
routing loops
preset/channel serialization assumptions
host-test expectations
README / validation docs
```

Expected final parameter assertion:

```cpp
static_assert(kMaxParams == 234,
              "11-channel parameter budget changed");
```

Keep:

```cpp
static_assert(kMaxParams <= 241,
              "disting NT supports at most 241 parameters per algorithm");
```

Expected structural values:

```text
kNumGlobalParams  = 69
kNumChannelParams = 15
channels          = 11
kMaxParams         = 234
```

Do not accidentally allocate per-channel delay buffers for the ducker.
SC Lookahead and Bypass Offset remain stereo submix delays.

---

# 3. 11-channel acceptance tests

Run the existing build/test suite again.

Specifically verify:

```text
1. algorithm loads on disting NT at 11 channels
2. parameter table reports 234 parameters
3. all 11 channel pages appear
4. each channel still has independent MIDI/CV mapping identities
5. channel 11 routing works for:
   - Main
   - Bypass
   - Insert 1
   - Insert 2
   - FX Send 1
   - FX Send 2
6. no out-of-bounds writes in channel arrays
7. presets save/load all 11 channels
8. ducker still behaves identically
9. Master Filter and Master Gain remain unchanged
10. DRAM/SRAM requirements still fit
```

If 11 channels fails to load despite 234 parameters, investigate memory or a
separate NT runtime limit; do not assume the parameter count is the cause.

---

# 4. Bypass Offset UI placement — move it out of Sidechain

`Bypass Offset` is NOT conceptually a sidechain control.

It interacts automatically with `SC Lookahead`, but its purpose is:

```text
output/routing latency alignment
```

Therefore it belongs with the global output-routing controls.

Move its page/UI placement to the same output section where the user selects
or sees the Main/Bypass routing controls.

Preferred grouping:

```text
OUTPUT / ROUTING

Master Output
Main L
Main R
Bypass L
Bypass R
Bypass Offset
Master Send
Master Return
```

Exact ordering can follow the existing Witchboard page layout, but the rule is:

> Put `Bypass Offset` immediately next to the Bypass output/routing controls,
> not in the Sidechain section.

Do NOT change its parameter identity or DSP behavior merely because its UI page
moves.

---

# 5. Sidechain page after the move

The Sidechain section should contain only:

```text
Sidechain
SC Trigger Input
SC Depth
SC Lookahead
SC Env Length
SC Curve
SC Smooth
```

No `Bypass Offset` here.

This makes the UI match the actual conceptual split:

```text
Sidechain page:
    ducking behavior

Output / Routing page:
    where signal goes
    output latency alignment
```

---

# 6. Bypass Offset behavior remains unchanged

Only its UI/page placement changes.

Keep the v1.34 behavior:

```text
effectiveBypassMs =
    activeSClookaheadContribution + manualTrim
```

Where:

```text
activeSClookaheadContribution =
    Sidechain ON ? SC Lookahead : 0
```

Examples:

```text
SC ON
Lookahead = 6 ms
manual trim = +4 ms
Bypass Offset displays 10 ms
```

If Lookahead becomes 8 ms:

```text
Bypass Offset displays 12 ms
manual trim remains +4 ms
```

If SC is turned OFF:

```text
automatic 8 ms contribution is removed
manual trim remains +4 ms
Bypass Offset displays 4 ms
```

Keep preset-load protection so the auto contribution is not double-applied.

Keep MIDI/CV-follow behavior.

---

# 7. Bypass Offset routing behavior remains unchanged

## Split

```text
Bypass submix
-> Bypass Offset delay
-> physical Bypass L/R
```

## Sum

```text
Main submix
-> SC Lookahead
-> duck VCA
--------------------\
                     + -> master processing -> Main out
Bypass submix        /
-> Bypass Offset
--------------------/
```

## Insert

```text
Main submix
-> SC Lookahead
-> duck VCA
--------------------\
                     + -> master processing
Bypass submix        /
-> Bypass Offset
--------------------/
-> Master Send
-> Master Return
-> Main out
```

The UI move must not disturb this DSP placement.

---

# 8. Documentation changes

Update README and validation docs so they no longer say:

```text
supported maximum = 10 channels
```

Replace with:

```text
supported maximum = 11 stereo channels
```

Document the parameter budget:

```text
11 channels = 234 / 241
7 spare
```

Keep a note that:

```text
12 channels = 249 / 241
8 over
```

Also ensure the README's `Bypass Offset` description appears under the
Output/latency or Routing section, not inside the Sidechain control list.

---

# 9. Do not change

Do NOT alter:

```text
SC Depth law
SC Curve recurrence
SC Smooth mapping
SC Lookahead range
SC Env Length range/taper
trigger threshold
Master Filter
Master Gain
channel parameter identities
insert routing
FX sends
current Bypass Offset DSP semantics
```

This handoff is only:

```text
1. 10 -> 11 channels
2. move Bypass Offset UI/page placement
```

---

# 10. Final target state

```text
Witchboard v1.35 target

11 stereo channels
234 / 241 parameters

Sidechain page:
    Sidechain
    SC Trigger Input
    SC Depth
    SC Lookahead
    SC Env Length
    SC Curve
    SC Smooth

Output / Routing page:
    existing output controls
    Bypass Offset placed beside Bypass routing/output controls
```

12 channels remain a future parameter-budget problem:

```text
249 - 241 = 8 parameters over
```

Do not compromise the current direct/mappable channel architecture just to
force 12 channels in this pass.

---

# 11. Sidechain factory defaults — LOCKED in v1.36

The sidechain remains **OFF by default**, but the stored/default shaping values
should be the musical settings that worked well in hardware testing so users hear
a useful pump immediately when they assign a trigger and enable Sidechain.

Lock these defaults:

```text
Sidechain       OFF
SC Trigger      unassigned / existing safe default
SC Depth        69%
SC Lookahead     6.0 ms
SC Env Length   ~300 ms
SC Curve        +20
SC Smooth        4   (= 8 ms)
```

Implementation notes:

```text
- Keep Sidechain OFF at plugin load/default state.
- Do not change the trigger-input safety/default behavior just to force a sound.
- The user should only need to assign a trigger input and enable Sidechain to
  hear the intended pump.
- SC Env Length uses the existing logarithmic 50..2000 ms mapping; use the
  normalized/default value corresponding to approximately 300 ms.
- SC Smooth 4 means 8 ms using the locked JoyDuck-derived mapping.
```

These are now the preferred factory values because they were validated by ear
in the actual 120–130 BPM performance range and produced a strong but usable
pump without fully muting the Main path.

At 69% Depth, the theoretical deepest gain is:

```text
1 - 0.69 = 0.31
≈ -10.2 dB
```

Do not keep tuning these by theory unless hardware testing reveals a real
problem. They are intentionally being locked as a practical musical default.


---

# 11. Master Filter factory defaults — LOCKED

Change **defaults only**. Do not change the available parameter ranges or DSP behavior.

Factory defaults:

```text
Master Filter Q        = 10
Master Filter Lo Limit = 20%
Master Filter Hi Limit = 70%
```

These replace the previous factory/default values.

Do not alter:

```text
Master Filter Q range
Master Filter Lo Limit range
Master Filter Hi Limit range
filter topology
filter DSP
parameter mapping behavior
```

This is strictly a default-value change.
