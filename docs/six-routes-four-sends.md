# Six routes, four sends

WitchboardX supports 1–10 channels, six insert routes (A–F), and four complete FX send/return setups. Ten channels use **241 NT parameters**: 89 globals plus 15 per channel and two insert return timing controls on Final Outputs. See [insert return timing](insert-return-timing.md) for the appended IDs and preset behavior. Repeat Protection remains switchable. Switch fade appears only on Global.

## Send editor

Each channel has **Send select** and **Send amount** in place of its old FX1/FX2 mix parameters. Send select chooses FX1–FX4 (underlying values 0–4, with 3 and 4 both selecting FX4); its display uses the saved FX names. Send amount edits that send from 0–100. Selecting another send recalls its amount without changing any send level. All four levels remain active and are saved in the preset.

All four sends preserve the existing mix curve: 0–50 increases the wet send from zero to unity while retaining dry at unity; 50–100 keeps the send at unity while reducing dry to zero. The four dry factors multiply. Each wet feed is independent of the other send amounts. Switch fade smooths amount changes, including changes from MIDI. These are the existing dry/wet mixes, not a newly introduced conventional auxiliary-send law.

Each send retains send L/R, send width, return L/R, return width, and return path (Main/Bypass). There are no extra return-level controls.

## MIDI configuration

Both controller arrangements can be used together:

- Map NT's **Send select** and **Send amount** normally to choose a send and adjust it with one fader. Set the NT MIDI mapping range to **0–4**. For a four-state CC button use **0, 42, 85, 127**, selecting Send 1, 2, 3, 4 respectively. Values 3 and 4 both select Send 4, matching the insert selector endpoint behavior.
- Give each internal send its own channel/CC assignment for independent faders, regardless of the selected send.
- Optionally assign a custom **shared** fader that follows Send select and supports pickup. The plugin only applies its movement after it reaches/crosses the selected stored level. Selecting another send or changing that level from another control re-arms pickup.

Custom assignments are configured in preset JSON using the helper below. They are not extra NT parameters and do not appear in NT's normal mapping list. There is no panel MIDI-learn interface in this build. The NT-mapped Send amount remains available for ordinary panel/CV/MIDI editing; plugin pickup applies to the custom CC assignments, not to NT's own mapping system.

Example: give Witchboard channel 2 an independent FX3 fader on MIDI channel 3, CC21:

```sh
python3 scripts/configure_send_midi.py \
  presets/WitchboardX.json /tmp/witchboard-faders.json \
  --channel 2 --target 3 --midi-channel 3 --cc 21
```

Add a shared fader with pickup, keeping that independent assignment:

```sh
python3 scripts/configure_send_midi.py \
  /tmp/witchboard-faders.json /tmp/witchboard-shared.json \
  --channel 2 --target shared --midi-channel 3 --cc 22 --pickup
```

The helper writes a new file and refuses to overwrite an existing destination. Load the resulting preset on NT. `--minimum` and `--maximum` default to 0 and 100; reversed ranges are supported. MIDI channel 0 disables an assignment. Supported messages are ordinary 7-bit CCs 0–119 on channels 1–16; note messages and channel-mode CCs are ignored. Use different channel/CC pairs for unrelated controls, including existing NT mappings. Reusing the same pair intentionally controls every matching target.

Within the Witchboard slot, `witchboardSendLevels` contains one four-number array per channel. `witchboardSendMidi` contains one array per channel, with five entries ordered **FX1, FX2, FX3, FX4, shared**. Each entry is:

```json
[3, 21, 0, 100, 1]
```

The fields are MIDI channel, CC, minimum amount, maximum amount, pickup (0/1). Disabled default: `[0,0,0,100,0]`. All assignments and levels serialize on NT save. Pickup position is transient and starts uncaught after loading.

## Preset compatibility and migration

Parameter identities changed. **Use a preset matching this layout; presets from other Witchboard versions are not interchangeable.** This build keeps the WitchboardX GUID `WtbX`, so it replaces the existing WitchboardX binary rather than installing as a second algorithm. Keep a backup of the old binary and preset if you need to switch back.

The included `presets/WitchboardX.json` is configured for ten channels, six routes, and four sends. FX3/FX4 begin at zero with unassigned endpoints; route/FX assignments depend on your hardware patch. Check bus assignments before using the preset with connected equipment.

The migration helper supports the personal eight-route/two-send preset schema
with up to ten channels. It preserves other algorithms and refuses configured
Route G/H assignments or unsupported mappings rather than silently dropping
them. It does not convert older public-release schemas. The resulting preset
needs the two appended insert route timing controls `[1, 0]` and six zero
`witchboardInsertReturnOffsets` values. Back up the source preset before converting it.

To migrate a supported personal eight-route/two-send preset:

```sh
python3 scripts/migrate_six_routes_four_sends.py source.json destination.json --channels 10
```

Omit `--channels` to retain its channel count. The converter requires the
87-global personal schema and cannot reduce channel count. New send levels do
not have individual native CV/NT mapping identities; dedicated MIDI control is
handled by the plugin.

## Verification

`make verify` builds the ARM plugin and checks its imports, runs the existing routing/gain/ducker tests plus four-send tests at 32, 44.1, 48 and 96 kHz, and tests preset conversion. Send tests cover simultaneous stereo feeds and returns, mono handling, independent/shared CCs, pickup, selected amount synchronization, save/load callback ordering, and unique page membership. NT hardware loading, controller feel, and CPU use still require an audition on the device.
