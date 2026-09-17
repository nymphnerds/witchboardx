WitchboardX v1.0.5 adds shared insert returns and insert route latency alignment (NT Gallery identity `WtbX`, distinct from Witchboard `WtC1`). Up to ten stereo channels can use six insert routes and four separate stereo Send FX paths.

Channels selecting the same insert route share its send and have the physical return mixed once. Each insert route has an independent 0–20 ms return offset; the largest active offset sets the timing reference for the rest of the mix. SC Lookahead and Bypass Offset appear together on the Latency page, and Bypass Offset follows active lookahead unless manually overridden. Audio-loop work for inactive sends and insert routes has been reduced.

This build is the no-DRAM firmware-safe release for **disting NT v1.18 or later**.

Download **WitchboardX.zip** or **release.zip** for the plugin object only. No user preset or sample library is included. Copy `programs/plug-ins/WitchboardX.o` to the same path on the MicroSD card.

This release contains the plugin object only. It does not include a user preset or sample library. Older WitchboardX presets use a different parameter layout; load a preset saved for this build.

Four-rate routing/DSP/preset tests, preset migration tests, ARM object build and object inspection run before packaging. In one on-device test at 44.1 kHz with nine channels, active latency offsets, sidechain and filter enabled, the WitchboardX algorithm showed 26% CPU; use and routing affect this reading.
