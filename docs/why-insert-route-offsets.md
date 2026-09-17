# Shared insert returns and timing in WitchboardX

## One insert, several channels

An **insert** sends audio out of WitchboardX to an external processor, then brings the processed audio back. You can route several mixer channels through the same insert—for example, an iPad running a stack of effects. Their audio is summed on the insert send, so the iPad receives one combined signal and produces one return.

That return must be mixed **once**, even if three channels sent audio to it. Mixing it once for each channel would make it too loud. WitchboardX handles this automatically when the channels select the **same insert route** (A–F). It looks at their route selections, including routes active during a switch, and reads that route's return once. It does not inspect the audio or recognise that two different route names happen to use the same physical bus. To share an insert return, select the same route on those channels.

The external processor has already combined the channels, so its return cannot be split back into separate channel signals. This is a shared **insert**, separate from WitchboardX's four **Send FX** paths. If the shared return also feeds a Send FX path, WitchboardX uses the highest send amount requested by its contributing channels; it does not add those amounts together.

## Keeping the return in time

An iPad or computer takes time to process and return audio. Without compensation, its processed signal can arrive behind the channels that stayed inside WitchboardX. On the final **Offset** page, select the **Insert route** and set **Insert return offset** to that route's round-trip delay. All channels using that route share the same setting.

WitchboardX takes the largest offset among active insert routes as its timing reference and delays faster paths to line them up with the slowest return. It cannot make an external return arrive earlier. **Bypass Offset** is on the same page; **SC Lookahead** remains with the sidechain settings. Send FX returns have no separate offset control in this build.

In my setup, an insert bus runs through a stack of iPad effects. A **6.0 ms insert return offset** brings it into line with the dry mix; to my ears, it is rock tight and solid.

## Why the offset is per route

An earlier design gave every track its own offset. On my disting NT, earlier offset builds pushed WitchboardX toward **40% CPU** and eventually cut out; with Poly Res enabled, total NT CPU could hit **99%**. The delay I needed to correct belonged to the external insert route, not to each track separately, so a route setting was a better fit.

The later build also reduced other processing work, especially for inactive Send FX and shared returns. In a 44.1 kHz, ten-channel test with Poly Res enabled, WitchboardX measured **23% CPU at 0 ms** and **28% at an 18.2 ms insert offset**, with no cutouts reported during that test. Those improvements cannot be credited to the offset change alone. The 6.0 ms iPad setting above is my current listening result, not the setting used for those CPU figures.
