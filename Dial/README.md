# Dial

Dial is a small personal native C++ plug-in for the Expert Sleepers disting NT.
It selects Loopy Pro control profiles from the NT and mirrors Loopy's current
profile back to the NT over MIDI CC.

It appears on the NT as `Dial` and builds to `dial.o`.

## Profile Names

Dial stores preset-specific profile names in the plug-in JSON member
`dialNames`, intended for NT Helper/editing workflows.

Example:

```json
"dialNames": [
  "Intro",
  "Main",
  "Breakdown",
  "Outro"
]
```

Missing names fall back to `Profile 1`, `Profile 2`, and so on. Dial stores up
to 32 names.
