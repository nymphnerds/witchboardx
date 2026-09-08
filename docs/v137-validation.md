# Witchboard v1.37 implementation and validation

This applies the cumulative [v1.37 handoff](handoffs/Witchboard_v3_IMPLEMENTATION_HANDOFF_v137_11Channels_BypassOffset_SC_FilterDefaults.md).
The file retains a v1.35 opening title and appends the v1.36 sidechain defaults
and final filter defaults; all of those requirements are included here.

## Current target

- Supported and initially selected channel count: **11 stereo channels**.
- Parameter budget: **69 globals + 11 × 15 = 234 / 241**, seven spare.
- Twelve channels remain unsupported: 249 parameters, eight over the limit.
- Bypass Offset remains plugin parameter **68**. Its page placement is now
  **Final Outputs: Main L, Main R, Bypass L, Bypass R, Bypass Offset**.
- Sidechain/Master begins with the seven ducking controls, followed by the
  existing filter and master controls. Bypass Offset is absent from that page.
- Channel 11 uses indices 219–233. All earlier channel and global identities
  remain unchanged. Its page label and parameter prefix are `Channel 11`/`11:`.

| Factory control | Default |
| --- | --- |
| Sidechain | Off |
| SC Trigger Input | Unassigned |
| SC Depth | 69% |
| SC Lookahead | 6.0 ms |
| SC Env Length | Approximately 300 ms (normalized value 486) |
| SC Curve | +20 |
| SC Smooth | 4 (8 ms) |
| Filter Q | 10 |
| LP limit (Lo Limit) | 20% |
| HP limit (Hi Limit) | 70% |

Only parameter defaults changed: ranges, filter topology, trigger detection,
curve recurrence, smoothing, VCA depth law and latency semantics are retained.
A source comparison with the pre-change v1.34 snapshot confirmed all DSP,
routing, parameter callback and serialisation function bodies are unchanged.
The NT API and MCP references remain documented in the
[v1.34 implementation notes](v134-validation.md#api-and-mcp-references).

## Presets and mapping

Existing v1.34 presets load without parameter-value migration. The new factory
values do not overwrite their saved shaping/filter settings or mappings. Their
channel-count specification also remains as saved. A stored master-page cursor
can select a different row because Bypass Offset moved to Final Outputs.

The converter retains its `migrate_v134_preset.py` name because the 69-global
parameter schema is unchanged. It now accepts 11-channel presets. Converting
older pre-v1.34 layouts still preserves compatible channel/routing/filter/gain
values and mappings, and creates the replacement ducker with Sidechain Off;
new Curve controls use +20. Existing v1.34 layouts pass through unchanged.

## Automated checks

`make package` runs the existing routing suite, four-rate gain/ducker suites,
the new four-rate channel suite, Python preset migration tests, ARM compilation,
and object/import/DRAM-section inspection before packaging.

`WitchboardChannelsTest` runs at 32, 44.1, 48 and 96 kHz and verifies:

1. Eleven-channel construction, 234 parameters and all eleven named pages.
2. Independent channel indices and correct channel 11 labels/prefixes.
3. Bypass Offset's new location and unchanged identity.
4. Every locked factory default, including safe SC Off/unassigned trigger.
5. Channel 11 stereo Main, Bypass, Insert 1, Insert 2, FX Send 1 and FX Send 2
   paths, while another channel remains independent.
6. Host-style save/load of all 234 values and plugin metadata, with identical
   output after reloading each routing case.
7. Unchanged SRAM size, channel-only DRAM growth and both delay rings inside the
   allocated DRAM region.

Python tests also exercise eleven-channel presets containing MIDI/CV mapping
objects, preserving all channel mappings and all unrelated slots. The existing
four-rate suites cover the unchanged envelope, delay, auto-follow, master filter
and gain behaviour.

Host memory is 640 bytes SRAM and 86,544 bytes DRAM for eleven channels, compared
with 86,368 bytes DRAM for ten: the extra channel adds 176 host bytes. The two
stereo delay rings remain 84,496 bytes total and do not grow with channel count.
Host structure sizes do not establish target firmware memory availability.

The 96 kHz channel suite also passed AddressSanitizer and UndefinedBehaviorSanitizer,
covering channel-11 array access, routing and preset restoration. As with the
previous run, LeakSanitizer is disabled because the environment uses ptrace;
no leak-check result is claimed.

## Hardware acceptance still required

Host construction is not a disting NT load test. Load the eleven-channel object
on the intended firmware and confirm all eleven pages, CPU use, allocation,
MIDI/CV mapping, preset round-trips, and the locked audible defaults. A load
failure at 234 parameters would need memory/runtime investigation; it should
not be attributed to the 241-parameter limit without evidence.
