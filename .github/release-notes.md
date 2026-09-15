WitchboardX v1.0.3 is a firmware-safe release of this separate plugin (NT Gallery identity `WtbX`, distinct from Witchboard `WtC1`). It supports up to 10 stereo channels, six insert routes, four shared stereo FX sends, per-channel timing offsets, trigger ducking, Main lookahead, Bypass Offset alignment, master filtering, JSON-backed names, and route-derived insert slot labels.

This build is the no-DRAM firmware-safe release for **disting NT v1.18 or later**.

Download **release.zip** for the plugin, installation instructions, migration/MIDI tools, and the included WitchboardX example preset. The example's samples and external hardware are not included. Copy `programs/plug-ins/WitchboardX.o` to the same path on the MicroSD card.

The included preset demonstrates `witchboardNames.channels`, `routes`, `fx`, automatic insert-slot naming, four-send settings, and channel offsets. The supplied migration scripts support the personal eight-route/two-send layout described in the package documentation; they do not convert every older Witchboard preset.

Four-rate routing/DSP/preset tests, preset migration tests, ARM object build and object inspection run before packaging. Hardware audio and CPU use still need checking on the disting NT.
