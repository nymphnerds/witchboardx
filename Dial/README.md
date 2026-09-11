# LoopyDial

LoopyDial is a small personal native C++ plug-in for the Expert Sleepers disting NT.
It selects Loopy Pro control profiles from the NT and mirrors Loopy's current
profile back to the NT over MIDI CC.

It appears on the NT as `LoopyDial` and builds to `LoopyDial.o`.

## Profile Names

LoopyDial stores preset-specific profile names in the plug-in JSON member
`loopyDialNames`, intended for NT Helper/editing workflows. It can still read
the older `dialNames` key for existing presets.

Example:

```json
"loopyDialNames": [
  "Intro",
  "Main",
  "Breakdown",
  "Outro"
]
```

Missing names fall back to `Profile 1`, `Profile 2`, and so on. LoopyDial stores
up to 32 names.
