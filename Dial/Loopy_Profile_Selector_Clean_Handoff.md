# Loopy Profile Selector for disting NT — Clean Implementation Handoff

## Goal

Create a **small native C++ plug-in for Expert Sleepers disting NT** that acts as a **two-way Loopy Pro control-profile selector**.

The plug-in should live on the disting NT like a normal utility algorithm and let the user select Loopy Pro control profiles from the NT itself.

Design priorities:

```text
simple
stable
native-feeling
low CPU
low memory
no DSP
```

Do not add unrelated features.

---

## Parameters

### Profile page

```text
Profile
Profile count
Home
```

### MIDI page

```text
MIDI channel
MIDI CC
```

Profiles are numbered only:

```text
Profile 1
Profile 2
Profile 3
...
```

No editable profile names are required.

---

## Core behaviour

### Profile

`Profile` selects the active Loopy Pro control profile.

Conceptual NT-side range:

```text
1 .. Profile count
```

When `Profile` changes on the disting NT:

1. convert the selected zero-based profile index to the normalized MIDI value Loopy Pro expects;
2. send that value as a MIDI CC message;
3. Loopy Pro switches to the corresponding control profile.

### Profile count

Range:

```text
1 .. 32
```

This value controls both:

```text
1. the profile count used by the encode/decode math
2. the actual NT parameter ranges of Profile and Home
```

Changing `Profile count` must dynamically rescale:

```text
Profile -> 1 .. Profile count
Home    -> 1 .. Profile count
```

### Home

Stores the preferred home/default profile number.

Conceptual range:

```text
1 .. Profile count
```

For the first implementation, it is enough that `Home` is stored and adjustable as a normal parameter.

Do not invent front-panel shortcuts.

### MIDI channel

Range:

```text
1 .. 16
```

Default:

```text
1
```

### MIDI CC

Range:

```text
0 .. 127
```

Default:

```text
119
```

Use the same CC for outgoing selection and incoming Loopy state feedback.

---

## Exact Loopy Pro normalization

Use this exact mapping:

```cpp
inline int encodeProfile(int index, int count)
{
    if (count <= 1)
        return 0;

    const int d = count - 1;
    index = clamp(index, 0, d);

    return clamp(
        (128 * index + d / 2) / d,
        0,
        127
    );
}
```

Observed values:

```text
2 profiles: 0, 127
3 profiles: 0, 64, 127
4 profiles: 0, 43, 85, 127
```

Do not replace this with a simpler formula unless testing proves identical results for all counts.

---

## Incoming MIDI feedback

Listen for incoming MIDI CC matching:

```text
configured MIDI channel
configured MIDI CC
```

When a matching CC arrives:

1. compare it against the valid encoded values for the current `Profile count`;
2. choose the nearest profile;
3. update the NT `Profile` parameter.

This provides:

```text
NT -> Loopy
Loopy -> NT
```

---

## Feedback-loop suppression

When `Profile` is changed because of incoming Loopy feedback, do not immediately echo the same value back.

Use a minimal pending-feedback value/flag.

Inside `parameterChanged()`:

```text
if Profile change matches pending feedback:
    clear pending feedback
    do not send MIDI
else:
    send MIDI
```

---

## Scope

This plug-in does not process audio.

Expected requirements:

```text
no audio inputs
no audio outputs
no DSP state
tiny SRAM
0 DRAM unless genuinely needed
0 DTC
0 ITC
```

No heap allocation.

Prefer fixed-size state.

---

## disting NT implementation requirements

Use the official API:

```text
https://github.com/expertsleepersltd/distingNT_API
```

Use ordinary NT parameter pages and normal front-panel behaviour.

Avoid custom UI unless absolutely necessary.

Preferred callbacks:

```text
calculateRequirements
construct
parameterChanged
midiMessage
```

Use `step()` only if a small one-time deferred initialization is actually required by the API lifecycle.

Keep these `NULL` unless genuinely needed:

```text
hasCustomUi
customUi
setupUi
serialise
deserialise
midiSysEx
parameterUiPrefix
```

A draw callback is optional only if it adds value without interfering with normal NT behaviour.

---

## Parameter display

For `Profile` and `Home`, displaying:

```text
Profile 1
Profile 2
Profile 3
...
```

is desirable.

If the public API supports this cleanly through `parameterString()`, use it.

Do not implement editable names.

Do not create helper parameters such as:

```text
Rename profile
Name position
Name character
```

---

## Parameter ranges — non-negotiable

The actual disting NT parameter ranges must dynamically scale to `Profile count`.

Required behaviour:

```text
Profile count = N

Profile range = 1 .. N
Home range    = 1 .. N
```

Examples:

```text
Profile count = 2
Profile: 1..2
Home:    1..2

Profile count = 6
Profile: 1..6
Home:    1..6

Profile count = 16
Profile: 1..16
Home:    1..16
```

This is **not** optional.

Do **not** leave `Profile` or `Home` at a fixed `1..32` range with internal clamping.

When `Profile count` changes:

1. update the `Profile` parameter maximum to the new count;
2. update the `Home` parameter maximum to the new count;
3. if the current `Profile` value is now above the new maximum, clamp it to the new maximum;
4. if the current `Home` value is now above the new maximum, clamp it to the new maximum;
5. update the NT parameter definitions using the documented API/lifecycle-safe method.

Use the official Expert Sleepers API/examples to verify the correct use of runtime parameter-definition updates.

The implementation must ensure these updates happen only when the algorithm is in a lifecycle state where the host calls are valid.

---

## Lifecycle and safety

Be conservative with host callbacks.

Important rules:

```text
do not assume NT_algorithmIndex() is valid during construction
do not call host parameter-update APIs before the algorithm is registered
do not mutate parameter definitions from arbitrary contexts
do not allocate in the audio callback
do not use dynamic allocation
```

If host interaction must be deferred until registration is complete, use a minimal one-time initialization path.

---

## Build target

Use GNU ARM C++:

```text
arm-none-eabi-c++
```

Recommended flags:

```text
-std=c++11
-mcpu=cortex-m7
-mfpu=fpv5-d16
-mfloat-abi=hard
-mthumb
-Os
-fPIC
-fno-rtti
-fno-exceptions
-Wall
-Wextra
-Werror
```

Build as a relocatable object:

```sh
arm-none-eabi-c++ \
  -std=c++11 \
  -mcpu=cortex-m7 \
  -mfpu=fpv5-d16 \
  -mfloat-abi=hard \
  -mthumb \
  -Os \
  -fPIC \
  -fno-rtti \
  -fno-exceptions \
  -Wall -Wextra -Werror \
  -I/path/to/distingNT_API/include \
  -c loopy_profile_selector.cpp \
  -o loopy_profile_selector.o
```

Do not link into an executable.

Deliverable is the `.o` plug-in object.

---

## Plug-in entry point

Use the standard Expert Sleepers pattern:

```cpp
extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
    switch (selector)
    {
        case kNT_selector_version:
            return kNT_apiVersionCurrent;

        case kNT_selector_numFactories:
            return 1;

        case kNT_selector_factoryInfo:
            return data == 0
                ? reinterpret_cast<uintptr_t>(&factory)
                : 0;
    }

    return 0;
}
```

Use a unique four-character GUID.

Suggested algorithm name:

```text
Loopy Profile
```

Suggested description:

```text
Loopy Pro control profile selector
```

Tag:

```text
kNT_tagUtility
```

---

## Suggested state

Keep the instance minimal, for example:

```cpp
struct LoopyProfileSelector : public _NT_algorithm
{
    bool ready;
    int16_t feedbackProfile;
};
```

Only add mutable parameter definitions if dynamic ranges are actually used.

No profile-name arrays.

---

## MIDI send

Outgoing CC:

```text
status = 0xB0 | (channel - 1)
data1  = MIDI CC
data2  = encoded profile value
```

Use the official NT MIDI send API.

Use the destination that reaches the iPad/Loopy setup, expected to be USB for this system.

Confirm the current API constant from the official header before compiling.

---

## Testing

Test first in an empty/disposable preset.

### Test 1 — load/save/reload

```text
load plug-in
change parameters
save preset
restart/reload preset
```

Expected:

```text
no crash
```

### Test 2 — NT to Loopy

With:

```text
Profile count = 4
```

Expected outgoing values:

```text
Profile 1 -> 0
Profile 2 -> 43
Profile 3 -> 85
Profile 4 -> 127
```

### Test 3 — Loopy to NT

Expected incoming mapping:

```text
0   -> Profile 1
43  -> Profile 2
85  -> Profile 3
127 -> Profile 4
```

Also test nearby values to verify nearest-profile decoding.

### Test 4 — feedback suppression

Change profile from Loopy.

Expected:

```text
NT updates
NT does not immediately echo a duplicate CC back
```

### Test 5 — profile counts

Test:

```text
1
2
3
4
6
8
16
32
```

Verify encode/decode behaviour remains sensible and symmetrical.

---

## Non-goals

Do not add:

```text
audio processing
profile renaming
SysEx metadata
automatic discovery of Loopy profile count
automatic profile-name import
custom graphical UI
encoder shortcuts
MIDI learn
multiple MIDI destinations
preset-management features
```

---

## Deliverables

Produce:

```text
loopy_profile_selector.cpp
loopy_profile_selector.o
README.md
```

README should contain only:

```text
what the plug-in does
parameter list
default MIDI settings
normalization behaviour
build command
installation instructions
basic tests
```

Main success criterion:

```text
a boring, reliable, native-feeling utility plug-in
that selects Loopy Pro control profiles from the disting NT
and mirrors Loopy's current profile back to the NT.
```
