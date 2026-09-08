# Witchboard v1.34 implementation and validation

Historical results for v1.34. The current supported maximum is **11 channels /
234 parameters**; see [v1.37 validation](v137-validation.md) for the current
channel count, UI placement and factory defaults.

Implemented in `plugins/Witchboard/Witchboard.cpp` against the v1.34 handoff.
The existing master checkout and its DRAM code-placement fix were the baseline.
No channel-editor refactor or signed per-channel latency system is included.

## API and MCP references

- Official API checkout used for the ARM build: `../distingNT_API-v119`, commit
  `5a4910d1d4233180114d6aee5ddaa4b8aec577e8`.
- [Expert Sleepers API header](https://github.com/expertsleepersltd/distingNT_API/blob/5a4910d1d4233180114d6aee5ddaa4b8aec577e8/include/distingnt/api.h):
  per-instance `requirements.dram`/`pointers.dram`, `kNT_scaling10`,
  `kNT_unitHasStrings`, `parameterString`, `NT_algorithmIndex`,
  `NT_parameterOffset`, and `NT_setParameterFromAudio`. The latter is permitted
  from `step()`; the public Bypass Offset is updated through it, not a write to
  the host's const parameter array.
- The Disting NT MCP search was used for parameter-setting and deserialisation
  patterns, including [nt_multi_seq's audio parameter setter](https://github.com/thorinside/nt_multi_seq/blob/main/nt_seq_params.cpp),
  official gain custom UI examples, and community preset callbacks.
- [Cycling ’74 rampsmooth~](https://docs.cycling74.com/reference/rampsmooth~/)
  specifies restarting a linear ramp when the incoming value changes. The
  implementation finishes a constant-input transition in N samples, with equal
  rise/fall settings. Continuously moving inputs restart the ramp each sample.
- [Cycling ’74 curve~](https://docs.cycling74.com/reference/curve~/) and the
  handoff's coefficient candidate inform curve semantics. Exact Max operator
  fidelity has not been established.

The public API does not document the full preset parameter-notification order.
Auto-follow therefore runs only at audio-block boundaries. Construction and
`deserialise()` disarm it; the first subsequent block adopts the restored public
values without adding compensation. Host tests cover parameter notifications
before and after deserialisation, and loading into an already rendered instance.
On-device preset ordering and MIDI/CV integration still require verification.

## Implementation choices

- 69 globals, 15 parameters per channel, 219 at ten channels. Original channel
  field order, routing and channel count are retained. All master EQ state,
  parameters and DSP, plus old ducker modes/controls, are removed.
- Lookahead and Bypass Offset use tenths of a millisecond with `kNT_scaling10`.
  Env Length uses a normalized 0–1000 control and a millisecond display string,
  mapped as `50 * pow(40, x)`. Initial control 486 displays about 300 ms.
- A trigger outputs raw envelope zero on the trigger sample. The next N samples
  advance the recurrence, with exactly 1 forced at N. Length and Curve changes
  take effect on the next trigger. Smooth and Depth update each block.
- Double recurrence state avoids accumulated single-precision multiplier error
  over long envelopes. For `q - 1`, a sixth-order small-argument polynomial avoids
  cancellation (`|beta/N| < 0.031` over supported ranges). `powf(e, beta)` uses an
  existing NT libm import. There is no per-sample exponential or allocation.
- Both stereo rings are populated continuously, including at zero delay. This
  ensures history is available when enabling lookahead during ongoing audio.
  Each change crossfades taps over 5 ms; requests during a fade are coalesced to
  the latest target and start when that fade ends. Steady settings add the exact
  rounded sample delay. SC Off reaches zero Main delay after the transition.
- Bypass Offset remains the actual effective delay, including mapped controls.
  Manual trim survives physical clamping at 0/100 ms, using optional
  `witchboardLatencyTrim` preset metadata. The restored public value wins if the
  metadata is absent or inconsistent. Direct edits replace the previous trim.
  Simultaneous Bypass and automatic changes treat the edit as occurring against
  the previous automatic contribution before applying the new one.
- Two fixed stereo rings reserve 84,496 bytes of per-instance DRAM, independent
  of channel count. Host tests report 640 bytes SRAM and 84,792–86,368 bytes DRAM
  for 1–10 channels; host structure sizes are not target allocation sizes.
- Existing seven cold callbacks stay in `._nt_dram`; audio DSP stays in `.text`.

## Automated acceptance coverage

`make verify` runs the C++ suites and Python migration tests, builds the Cortex-M7
object and checks its format, imports and function placement.

| Handoff tests | Coverage |
| --- | --- |
| 1–7: envelope | Linear/positive/negative curves, monotonicity, exact endpoint, finite extremes, sample counts, comparison with a double-precision closed form within 1e-5 gain |
| 8–11: Smooth | Zero bypass, Smooth 4 = 8 ms, equal up/down ramps, moving curved input |
| 12–15: Depth | 0/50/100% minima and unity endpoints |
| 16–19: Lookahead | Stereo impulses at 0/6/10 ms, 96 kHz maximum, immediate trigger |
| 20, 28: live delays | 1 kHz signal derivative bounds under rapidly changing taps, repeated live plugin edits/toggles, exact zero-delay settling; listening still required |
| 21–23: Bypass | Stereo impulses through Split, Sum and Insert, FX-return branches, manual delay with SC Off |
| 24–27, 29–30: auto-follow | Enable/disable, manual trims, direct edits, callback and no-callback mapped-value changes, parameter setter with common offset and synchronous callback, load order, clamp round-trip via actual plugin serialisation callbacks |
| 31–34: routing/gain | FX-return and channel paths, Bypass filter placement in all modes, final gain after master return and additive output buses |

Ducker and gain suites run at 32, 44.1, 48 and 96 kHz. Additional checks cover
4-frame versus 64-frame processing equivalence, ring wraparound, instance
isolation, fractional controls and parameter display. The existing routing suite
still covers insert switching, route reuse protection and per-channel mapping.
The JSON test adapter models host token parsing and parameter notifications;
it does not execute the firmware itself.

Migration tests cover all previously supported layouts, mapping objects,
unchanged other slots, idempotency, incompatible layout rejection, and exclusive
creation of the destination file. The converter intentionally resets Sidechain
Off and drops obsolete EQ/ducker mappings, retaining compatible depth/input,
filter, final gain, routing, names and channel mappings.

The 96 kHz ducker/integration suite also passes AddressSanitizer and
UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`). LeakSanitizer cannot
run under this environment's ptrace/debugger wrapper, so that run used
`ASAN_OPTIONS=detect_leaks=0`; no leak-check result is claimed.

A separate `presets/Tricky plastic time-v134.json` was generated from the supplied
preset. The original file is preserved. The converted copy starts with Sidechain
Off, as documented for legacy migration.

## Hardware follow-up

Load the object on the intended v1.19-capable firmware, using a new or migrated
preset. Confirm memory allocation and CPU use at the desired channel count and
sample rate, MIDI/CV Lookahead auto-follow, saved effective offset after loading,
and live tap transitions. Audition the handoff's kick/bass/pad/irregular-trigger
and external/iPad Sum-mode cases. Host derivative bounds do not guarantee
inaudibility for arbitrary program material, and exact JoyDuck/Max matching is
not claimed.
