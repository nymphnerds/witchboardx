# WitchboardX plugin handoff — custom channel names

Updated: 2026-09-11

## Goal

Add custom Witchboard channel names to the C++ plugin, similar to the existing custom route/FX/slot labels.

Current JSON already supports plugin-private names for:

```json
"witchboardNames": {
  "routes": ["Percall 1", "Pico MMF", "Steve's MS-22", "Kirbinator", "Spare"],
  "fx": ["Radiant", "iPad Send"],
  "slots": [
    ["Dry", "Slot 1", "Slot 2", "Slot 3"],
    ["Dry", "Slot 1", "Slot 2", "Slot 3"]
  ]
}
```

Desired addition:

```json
"witchboardNames": {
  "channels": [
    "Kick",
    "Snare",
    "Hats",
    "Perc",
    "Radio",
    "Chord",
    "Pico",
    "Pony",
    "Poly Res",
    "Perc+Breaks",
    "iPad Instr"
  ],
  "routes": ["Percall 1", "Pico MMF", "Steve's MS-22", "Kirbinator", "Spare"],
  "fx": ["Radiant", "iPad Send"],
  "slots": [
    ["Dry", "Slot 1", "Slot 2", "Slot 3"],
    ["Dry", "Slot 1", "Slot 2", "Slot 3"]
  ]
}
```

## Why this is needed

Live Disting NT parameter tools can rename normal algorithm slots, but Witchboard's internal channel/route/FX labels are plugin-private data and are not exposed as ordinary parameters.

Observed:

```text
search_parameters("name", slot 6) -> no matches
show_slot_metadata(slot 6) -> no visible editable channel name params
```

However, direct JSON editing confirms Witchboard private names exist under `witchboardNames`.

## Implementation suggestion

### 1. Add channel name storage

In plugin state, add something like:

```cpp
std::array<std::string, 12> channelNames;
```

or size dynamically based on Witchboard channel count.

Default fallback should be generic:

```text
Ch 1, Ch 2, Ch 3, ...
```

Do not hardcode patch-specific names as defaults.

### 2. Load from JSON

When reading `witchboardNames`, also look for `channels`:

```cpp
if (names.contains("channels")) {
    for (int i = 0; i < numChannels && i < names["channels"].size(); ++i) {
        channelNames[i] = names["channels"][i].get<std::string>();
    }
}
```

Fallback if absent or empty:

```cpp
channelNames[i] = defaultChannelName(i); // e.g. "Ch 1"
```

### 3. Save to JSON

When saving existing `witchboardNames.routes`, `witchboardNames.fx`, and `witchboardNames.slots`, also save:

```cpp
witchboardNames.channels
```

Only save as many names as the current channel count.

### 4. Use names in UI

Where the plugin currently draws or displays `Channel 1`, `1:Input`, etc., use:

```cpp
const std::string& label = channelNames[i].empty()
    ? defaultChannelName(i)
    : channelNames[i];
```

Suggested display examples:

```text
Kick
Kick Input
Kick Gain
Kick Insert 1
Kick FX Send 2
```

or shorter if display width is tight:

```text
Kick In
Kick Gain
Kick FX2
```

### 5. Editing approach

Minimum useful implementation:

- JSON-only channel names.
- Plugin reads/writes `witchboardNames.channels`.
- Users edit JSON externally.

Better implementation:

- Add a custom UI name editor if the Disting NT plugin API supports text entry.
- Still store names in JSON for portability.

Avoid exposing names as normal numeric parameters unless there is a suitable string parameter mechanism.

## Desired names for current preset

For current 11-channel `WitchboardX` patch:

```text
Ch1   Kick
Ch2   Snare
Ch3   Hats
Ch4   Perc
Ch5   Radio
Ch6   Chord
Ch7   Pico
Ch8   Pony
Ch9   Poly Res
Ch10  Perc+Breaks
Ch11  iPad Instr
```

Routes:

```text
Route A  Percall 1
Route B  Pico MMF
Route C  Steve's MS-22
Route D  Kirbinator
Route E  Spare
```

FX:

```text
FX1  Radiant
FX2  iPad Send
```

## Compatibility requirements

- Old presets without `witchboardNames.channels` must load normally.
- If fewer channel names are present than current channel count, default missing names.
- If more names are present than current channel count, ignore extras.
- Preserve existing `routes`, `fx`, and `slots` name behavior.

## Current routing context

Current patch uses this channel order:

```text
Ch1  Aux 1       Kick
Ch2  Aux 2       Snare
Ch3  Aux 3       Hats
Ch4  Aux 4       Perc
Ch5  Input 10    Radio
Ch6  Input 2     Chord
Ch7  Input 3     Pico
Ch8  Input 4     Pony
Ch9  Aux 9/10    Poly Resonator
Ch10 Aux 11/12   Perc Poly + Breaks
Ch11 Aux 13/14   iPad Instrument
```

Relevant docs in same directory:

- `WitchboardX.md`
- `wiring(3).md`
- `WITCHBOARDX_HANDOFF.md`
