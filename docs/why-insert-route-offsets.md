# Shared insert returns and timing in WitchboardX

## What makes the insert shared?

A conventional channel **insert** sends one channel to an external effect and brings that channel's return back into its signal path. It assumes the return belongs to that one channel. If two channels are sent to the **same physical insert** and the mixer treats the return as belonging to both, it mixes the *entire* processed return twice. Two channels sharing an iPad effect should not double its output just because both selected it.

WitchboardX lets several channels deliberately select one insert route (A–F). For example:

- Drums select route E, and a synth channel also selects route E.
- WitchboardX sums their outgoing audio onto route E's send. The iPad receives **drums + synth** and returns one processed signal.
- WitchboardX mixes that return **once for route E**, not once for drums and again for synth. The return therefore stays at its intended level.

WitchboardX knows the return is shared because it counts the channels currently using **route E**, including routes involved in an insert switch. The channels must select the **same WitchboardX route**: the plugin does not infer sharing by inspecting audio or noticing that two differently named routes use the same hardware bus.

The iPad's return is already a combined signal; it cannot be separated back into individual drum and synth returns. This shared **insert** is distinct from WitchboardX's four **Send FX** paths. If the shared return also feeds a Send FX path, WitchboardX uses the highest contributor send amount rather than adding their amounts together.

## Keeping the return in time

An iPad or computer takes time to process and return audio. Without compensation, its processed signal can arrive behind the channels that stayed inside WitchboardX. On the final **Latency** page, select the **Insert route** and set **Insert return offset** to that route's round-trip delay. All channels using that route share the same setting.

WitchboardX takes the largest offset among active insert routes as its timing reference and delays faster paths to line them up with the slowest return. It cannot make an external return arrive earlier. **SC Lookahead** appears on both the sidechain and Latency pages; **Bypass Offset** is beside it on Latency. Send FX returns have no separate offset control in this build.

In my setup, an insert bus runs through a stack of iPad effects. A **6.0 ms insert return offset** brings it into line with the dry mix; to my ears, it is rock tight and solid.

## Why the offset is per route

An earlier design gave every track its own offset. On my disting NT, earlier offset builds pushed WitchboardX toward **40% CPU** and eventually cut out; with Poly Res enabled, total NT CPU could hit **99%**. The delay I needed to correct belonged to the external insert route, not to each track separately, so a route setting was a better fit.

The later build also reduced other processing work, especially for inactive Send FX and shared returns. In a 44.1 kHz, ten-channel test with Poly Res enabled, WitchboardX measured **23% CPU at 0 ms** and **28% at an 18.2 ms insert offset**, with no cutouts reported during that test. Those improvements cannot be credited to the offset change alone. The 6.0 ms iPad setting above is my current listening result, not the setting used for those CPU figures.
