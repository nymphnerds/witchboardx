# Witchboard / disting NT Memory Allocation Investigation

**Target build:** Witchboard v3  
**Firmware analysed:** disting NT 1.18.0  
**Primary problem:** Witchboard loads at 10 channels, but changing the specification to 11 channels gives:

```text
Cannot respecify - insufficient memory
```

This document records the current evidence and provides controlled diagnostic builds to identify the exact limiting resource.

---

## 1. What Witchboard Requests

Current `calculateRequirements()`:

```cpp
void calculateRequirements(_NT_algorithmRequirements& requirements, const int32_t* specs)
{
    const int channels = clampInt(specs[0], 1, kMaxChannels);
    requirements.numParameters = kNumGlobalParams + channels * kNumChannelParams;
    requirements.sram = static_cast<uint32_t>(requiredSram(channels));
    requirements.dram = static_cast<uint32_t>(requiredDram(channels));
    requirements.dtc = 0;
    requirements.itc = 0;
}
```

So per-instance Witchboard currently requests:

```text
SRAM
DRAM
0 DTC
0 ITC
```

The plug-in object itself still has static/code sections outside these per-instance requests.

---

## 2. Current SRAM Allocation

Current source:

```cpp
size_t requiredSram(int)
{
    return sizeof(WitchboardAlgorithm);
}
```

The current tests report approximately:

```text
SRAM: 896 bytes
```

This is constant from 1 through 11 channels because the channel-dependent arrays are currently stored in DRAM rather than directly inside `WitchboardAlgorithm`.

The SRAM object contains the fixed master/runtime state including:

```text
SidechainRuntime
MasterFilterRuntime
MasterEqRuntime
route names
FX names
insert slot names
pointers to channel-scaled DRAM data
```

---

## 3. Current DRAM Allocation

Current source:

```cpp
size_t requiredDram(int channels)
{
    size_t size = 0;
    size = addStorage<_NT_parameterPage>(size, 5 + channels);
    size = addStorage<ChannelPage>(size, channels);
    size = addStorage<ChannelRuntime>(size, channels);
    return size;
}
```

The DRAM block therefore contains:

```text
1. parameter-page descriptors
2. per-channel parameter-index page arrays
3. per-channel runtime state
```

Construction currently mirrors that exactly:

```cpp
pageDefs = takeStorage<_NT_parameterPage>(dram, 5 + numChannels);
channelPages = takeStorage<ChannelPage>(dram, numChannels);
runtime = takeStorage<ChannelRuntime>(dram, numChannels);
```

---

## 4. Correct ARM / disting NT Memory Requirements

The earlier `2048 byte` figure came from the **x86-64 host test**, not from the Cortex-M7 target.

That host test reports host-side structure sizes, where pointers are 8 bytes. The disting NT is a 32-bit ARM Cortex-M7 target, where pointers are 4 bytes.

Therefore:

# the earlier 2048-byte DRAM-boundary hypothesis was based on the wrong architecture and is now ruled out.

Disassembly of the actual ARM plug-in object gives the real NT-side requirements.

### SRAM

```text
SRAM = 0x358 = 856 bytes
```

### DRAM formula

The ARM build effectively requests:

```text
align4(27 * channels + 60) + 136 * channels
```

This gives:

```text
10 channels = 1692 bytes DRAM
11 channels = 1856 bytes DRAM
```

So the actual 10 -> 11 increase is only:

```text
1856 - 1692 = 164 bytes
```

This is small.

There is **no 2 KiB boundary** being crossed by the real Cortex-M7 allocation.

This sharply reduces the likelihood that Witchboard's own DSP/runtime DRAM is the cause of the 11-channel failure.

---

## 5. Parameter Count

The current v3 build has:

```text
77 global parameters
15 parameters per channel
```

Therefore:

```text
10 channels:
77 + (10 × 15) = 227 parameters

11 channels:
77 + (11 × 15) = 242 parameters
```

Both are below the API's `uint8_t` parameter-index ceiling of 256.

The v3 README independently records the same current build count:

```text
77 globals + 15/channel = 242 parameters at 11 channels
```

which matches the source/test analysis used here.

However, the firmware itself contains an internal host-side structure named:

```text
_perParameter
```

so each parameter also consumes firmware-owned bookkeeping memory which is not included in Witchboard's own `requirements.sram/dram/dtc/itc` values.

This makes parameter count a second plausible limiting factor.

---

## 6. Expert Sleepers Dev / README Memory Notes

The end of the Witchboard v3 README contains Expert Sleepers developer guidance which should be treated as the primary platform-memory context for this investigation.

Reported current memory pools:

```text
ITC   ~64 kB
DTC   ~24 kB
DRAM  ~512 kB
```

These values were explicitly described as current implementation details rather than permanent API guarantees.

### Static object-section placement

The README/dev notes record the approximate mapping:

```text
.text                 -> ITC
.data                 -> DTC
.data.rel.ro           -> DTC
.rodata               -> DRAM
```

This is useful when inspecting the built `.o` file because static/code memory pressure is separate from the per-instance requests returned by `calculateRequirements()`.

### Per-instance memory

Expert Sleepers clarified that memory requested through:

```cpp
calculateRequirements()
```

comes from the larger global algorithm-memory system shared with built-in algorithms.

Therefore the small static DTC figure must **not** be treated as though it is also the complete budget for:

```cpp
req.sram
req.dram
req.dtc
req.itc
```

per-instance allocations.

In particular, do not diagnose Witchboard OOM by simply adding `.bss`/`.data` static usage to `calculateRequirements()` values and comparing that sum against the ~24 kB DTC figure.

### Work buffer

`NT_globals.workBuffer` lives in DTC.

It is temporary scratch memory for use during:

```cpp
step()
```

and does not persist between calls.

### Built-in algorithms

Built-in disting NT algorithms run mostly from flash/XIP, with only selected performance-critical code placed in ITC.

This explains why a custom plug-in's `.text` size can matter even when large amounts of DRAM remain available.

### Slow-code placement

The README/dev notes also mention Expert Sleepers' work on allowing slower plug-in code to be placed in DRAM, for example:

```text
filter/EQ initialisation
coefficient design
preset handling
other non-audio-rate code
```

This is now confirmed in the first disting NT v1.19 beta. The API provides `_NT_DRAM_SECTION`, which maps a function into the `._nt_dram` section.

### Practical OOM rule from the README

When investigating an NT memory failure, inspect both:

1. object-file sections
2. per-instance requirements from `calculateRequirements()`

Neither one alone gives the whole picture.

---

## 7. Superseded README Channel-Limit Assumption

The v3 README still contains an older hardware conclusion that effectively treated:

```text
11 channels
```

as the practical supported maximum because an earlier 12-channel build failed to load.

That assumption is now superseded by newer hardware testing.

Confirmed newer behaviour:

```text
10 channels = loads
11 channels = fails
```

The 11-channel failure also occurs in an otherwise empty preset.

The exact NT message is:

```text
Cannot respecify - insufficient memory
```

Therefore the current investigation must treat **10 channels** as the practical working maximum until the underlying allocation failure is identified.

The older README statement should not be used as evidence that 11 channels is valid on current hardware.

---

## 8. Firmware 1.18.0 Findings

The official 1.18.0 firmware image was inspected directly.

Relevant strings present in the firmware include:

```text
Cannot respecify - insufficient memory
sizeof(_algorithm) %u
sizeof(_perParameter) %u
Not enough memory to build global offset table:
Insufficient memory for static initialisation:
Not enough memory for factory:
```

The exact error observed on hardware is:

```text
Cannot respecify - insufficient memory
```

This confirms the failure is happening in the NT's algorithm **respecification** memory path.

The plain string:

```text
not enough memory
```

also exists in the firmware, but inspection showed that occurrence belongs to the embedded Lua runtime and is not the relevant Witchboard error path.

The firmware release is a stripped raw Cortex-M7/XIP image rather than a symbol-rich ELF, so exact internal allocation functions are harder to reconstruct, but the `_perParameter` diagnostic proves that host-side memory scales with parameter count.

---

---

---

# 9. Root Cause — Confirmed by Expert Sleepers

The 11-channel failure is now resolved.

Expert Sleepers developer **Os** confirmed:

> "You're over the max number of parameters by 1, I'm afraid."

For the current Witchboard v3 parameter layout:

```text
77 global parameters
15 parameters per channel
```

the totals are:

```text
10 channels:
77 + (10 × 15) = 227 parameters

11 channels:
77 + (11 × 15) = 242 parameters
```

The disting NT's actual maximum is therefore:

```text
241 parameters per algorithm
```

So:

```text
227 = valid
241 = maximum valid
242 = invalid
```

This explains both observed failure modes:

```text
10 channels = loads
11 channels = fails
```

and:

```text
respecify to 11
-> Cannot respecify - insufficient memory
```

as well as the diagnostic build that defaulted directly to 11 channels and failed to load.

The failure was **not** caused by Witchboard's own DRAM allocation, nor by temporary respecification overhead.

---

# 10. Why the Public API Ceiling Was Misleading

Witchboard previously treated the API's `uint8_t` parameter-page indices as implying a maximum of 256 parameters:

```cpp
static_assert(kMaxParams <= 256, "parameter page indices are uint8_t");
```

That is only an **addressing-range check**.

A `uint8_t` can represent:

```text
0 ... 255
```

but the firmware imposes a separate lower runtime limit:

```text
241 parameters maximum
```

Therefore:

# `uint8_t` index width does not define the real algorithm parameter limit.

The source-side assertion should be changed to:

```cpp
static_assert(kMaxParams <= 241,
              "disting NT supports at most 241 parameters per algorithm");
```

If Expert Sleepers later exposes a named API constant for this limit, use that instead of hard-coding `241`.

---

# 11. Corrected ARM Memory Findings

The earlier `2048 byte` figure came from the x86-64 host test, not from the Cortex-M7 target.

The actual ARM build requests approximately:

```text
SRAM = 856 bytes
```

and:

```text
10 channels = 1692 bytes DRAM
11 channels = 1856 bytes DRAM
```

So the 10 -> 11 increase is only:

```text
164 bytes
```

This was not the cause of the failure.

The earlier 2 KiB DRAM-boundary theory is therefore disproved.

---

# 12. Implication for 11-Channel Witchboard

11 channels are still feasible.

The current 11-channel total is:

```text
242 parameters
```

which is only:

```text
1 parameter
```

over the confirmed maximum.

Therefore the immediate fix is simply to remove or consolidate one exposed NT parameter.

Target:

```text
11 channels
241 parameters total
```

No DSP feature needs to be removed purely for memory reasons.

---

# 13. Implication for Future Compressor Work

The 241-parameter ceiling is now the main architectural constraint for future per-channel features.

A conventional compressor with 5 controls per channel would add:

```text
11 × 5 = 55 parameters
```

which is impossible with the current fully duplicated parameter model.

Future compressor design therefore needs one of these approaches:

```text
- reduce existing parameter count
- share/macro compressor controls
- use a selected-channel editor model
- move per-channel settings into Witchboard-owned internal state rather than exposing every setting as a separate NT parameter
```

The compressor DSP state itself is likely much cheaper than the parameter budget.

---

# 14. v1.19 Beta: Slow/Cold Code in DRAM

The first disting NT **v1.19 beta** now implements the facility that was
previously only discussed.

The API adds:

```cpp
#define _NT_DRAM_SECTION __attribute__ ((section ("._nt_dram")))
```

and the official `examples/gain.cpp` demonstrates:

```cpp
_NT_DRAM_SECTION
uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
    ...
}
```

The uploaded firmware contains:

```text
Firmware v1.19.0beta
._nt_dram
out of space for DRAM veneer
```

This confirms that the firmware loader recognises the dedicated DRAM function
section.

For Witchboard:

```text
hot real-time DSP     -> keep in fast code memory
cold/setup/UI/preset  -> candidates for _NT_DRAM_SECTION
```

Strong candidates include:

- `pluginEntry()`
- the large construction/setup path
- page/parameter setup helpers
- `serialise()`
- `deserialise()`
- `parameterString()`
- `parameterUiPrefix()`

Earlier ARM inspection found `constructWitchboard()` to be about 2 KiB of code
by itself, making construction/setup an especially useful target.

Do not automatically move `step()`, per-sample filter processing, or future
Dynamics3 audio processing into DRAM. Coefficient helpers should remain in fast
code memory if they are called from the audio path every block.

This optimisation does **not** change the 241-parameter limit. Its benefit is
code-memory headroom: larger Witchboard DSP and/or more simultaneously loaded
plug-ins when code memory is the limiting resource.

After adopting v1.19 beta, measure section sizes again and validate the result
on hardware.

API:
https://github.com/expertsleepersltd/distingNT_API

# slow-code placement is unrelated to the 241-parameter ceiling.

It may improve code-memory headroom, but it does not increase the number of NT parameters an algorithm may expose.

---

# 15. Final Conclusion

The issue is now solved.

```text
Actual disting NT algorithm parameter limit = 241
Current Witchboard 11-channel total        = 242
Difference                                 = 1
```

The previous assumption that the API allowed 256 parameters came from the `uint8_t` parameter index type and was incorrect as a runtime-capacity assumption.

The next implementation step is:

```text
reduce the 11-channel configuration by exactly one exposed parameter
```

and then verify that the algorithm loads successfully at 11 channels.


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
