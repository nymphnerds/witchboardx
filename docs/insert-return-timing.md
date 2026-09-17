# Insert return timing

This branch keeps three timing controls:

- **SC Lookahead** stays on **Sidechain/Master**.
- **Bypass Offset** is on the final **Offset** page and retains its SC auto-follow behavior.
- **Insert route** and **Insert return offset** are on the final **Offset** page. Select one of the six physical insert routes, then set its roundtrip offset from 0.0 to 20.0 ms. The six values save in the NT preset.

There is no per-channel offset control. FX sends and FX returns have no independent offset control in this build. The four Send FX systems still route normally.

The largest active insert route offset sets the common timing reference. The Main and Bypass dry sums and the sidechain key receive that delay; a deferred insert return receives the difference between the reference and its route's configured offset. A physical return shared by channels is read and mixed once. For a shared Send FX feed, the highest contributor send amount applies to that return. Zero offsets use the direct path.

This timing model assumes a route offset describes the effective latency of the final insert return. Two serial insert stages with independent roundtrip latency, especially when they share a final route with other channels, need separate hardware validation. The delay tap changes crossfade over 5 ms.

Ten channels use 241 plugin parameters. The two appended indices are 239 and 240, after the channel parameters; they now mean Insert route and Insert return offset. Older presets that used these for channel offsets should be backed up. Old `witchboardChannelOffsets` metadata is ignored, and missing new metadata initializes the six insert offsets to zero. The new metadata key is `witchboardInsertReturnOffsets`, containing six integers in tenths of milliseconds.

Host tests cover a single inserted channel, a shared physical return, editor save and restore, and legacy metadata at 32, 44.1, 48, and 96 kHz. NT hardware CPU and sustained audio with Poly Res enabled still need direct testing.
