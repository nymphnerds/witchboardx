# Per-channel timing offsets

The **Offset** page is after Channel 10 (or the last instantiated channel).
It contains two parameters:

- **Channel:** 1–10, limited to the instantiated channel count.
- **Offset:** −30.0 to 0.0 ms, in 0.1 ms steps. Each channel retains its own value.

All offsets default to zero. Select the channel with the late return and reduce
Offset until it lines up with the other channels. Save the NT preset to retain
every channel's offset.

## Timing

Negative offsets are relative to the mixer. Let `base` be the magnitude of the
most negative stored channel offset. Each channel receives `base + offset` delay.
For example, Channel 2 at −12 ms and all others at zero means Channel 2 receives
no additional delay and the others receive 12 ms. This aligns late audio; it does
not remove the iPad's latency relative to an external clock or live performance.

Delays run **after both inserts and before the four FX sends and Main/Bypass
split**. Insert sends retain their existing timing. Shared FX returns are not
additionally delayed: the channel feed into those FX already includes its offset.
The sidechain trigger receives the common `base` delay; existing SC Lookahead and
Bypass Offset then operate as before. This follows a zero-offset trigger source.
If the trigger source itself has a negative offset or external latency, its
relationship to ducking may require a separate timing adjustment.

Every instantiated channel contributes to `base`, including disabled channels,
so muting a channel does not move the rest of the mix. Reset an unused channel's
offset to zero if it should no longer establish the timing reference.
Offset changes use a 5 ms tap crossfade; changing timing during playback can
briefly blend two positions in the audio. Actual delays round to whole samples.
All-zero settings add no audio delay. Ring history remains active at zero for
smooth subsequent adjustments.

The new stereo channel/trigger rings use about 248 KiB of DRAM at ten channels,
plus parameter definitions and runtime state. No allocation occurs in the audio
loop. Hardware CPU use and perceived latency still need testing on the NT.

## Parameter and preset compatibility

Existing parameter IDs and page positions have **not moved** relative to the
six-route/four-send build. The editor is appended after the channel parameters:

| Ten-channel build | Plugin index (zero-based) | Preset index including NT common parameter |
|---|---:|---:|
| Channel | 239 | 240 |
| Offset | 240 | 241 |

The build uses **241 plugin parameters**. With fewer channels the two new IDs
follow the last instantiated channel, preserving all earlier IDs there too.
`witchboardChannelOffsets` stores one integer per channel in tenths of ms, from
−300 through 0. The selected channel and visible offset are ordinary parameters.
Old metadata without the array defaults all channel offsets to zero.

`presets/WitchboardX.json` has the two appended values `[1, 0]` and ten zero
stored offsets. Its previous parameter entries, mappings, names, UI state,
routing, and other algorithm slots are unchanged.

To align another **six-route/four-send** preset:

```sh
python3 scripts/migrate_channel_offsets.py old.json new.json
```

The helper preserves existing offsets if already aligned and refuses to overwrite
a destination. Older eight-route layouts need their separate migration first.

## Installation and verification

Install the v1.0.3 `WitchboardX.o` from the release package and load a preset
matching this parameter layout. The GUID remains `WtbX`, so the object replaces
the existing WitchboardX algorithm. The repository's example preset starts all
channel offsets at zero.

Validation: `make verify` runs the existing audio/preset tests plus offset editor,
save/restore callback ordering, parameter layout, stereo impulse, insert return,
FX send, Split/Sum/Insert, sidechain alignment, and live offset tests at 32, 44.1,
48, and 96 kHz. ARM inspection verifies the object format, plugin entry point and
host symbol dependencies. These are host/build checks; on-device testing remains.
