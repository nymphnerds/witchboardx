WitchboardX v1.0.2 is a full firmware-safe release of this separate plugin (NT Gallery identity `WtbX`, distinct from Witchboard `WtC1`). It supports 11 stereo channels with 234 parameters, trigger ducking, Main lookahead, Bypass Offset alignment, master filtering, JSON-backed channel/route/FX names, and route-derived insert slot labels.

This build is the no-DRAM firmware-safe release for **disting NT v1.18 or later**.

Download **release.zip** for the plugin, installation instructions, preset converter and aligned WitchboardX example preset. The example's samples and external hardware are not included. Copy `programs/plug-ins/WitchboardX.o` to the same path on the MicroSD card.

The included baseline preset demonstrates `witchboardNames.channels`, `routes`, `fx`, and auto slot naming. Existing v1.34 presets retain their saved values. Older layouts need the included converter.

Four-rate routing/DSP/preset tests, preset migration tests, ARM object build and object inspection passed before packaging.
