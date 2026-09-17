# Why WitchboardX uses insert return offsets

The earlier design gave every mixer track its own timing offset. That was more control than this setup needed: the audible delay comes from the external **insert** round trip, so it makes more sense to set the offset on the physical insert route. If several tracks share that route, they share its timing setting too. This also let the timing system avoid work for individual tracks that have no external latency to correct, helping keep WitchboardX light on the disting NT.

The final **Offset** page now has **Bypass Offset**, **Insert route**, and **Insert return offset**. I select the insert route and set the measured round-trip delay once. **SC Lookahead** stays with the sidechain controls. WitchboardX uses the largest active insert offset as its timing reference and holds the faster paths back to meet the late return. The offset cannot make an external processor return audio earlier; it aligns the rest of the mix with it.

In my patch, an insert bus goes out to an iPad and through a stack of effects before returning to WitchboardX. Setting **Insert return offset** to **6.0 ms** on that route brings the processed return into time with the dry paths. In my testing, the result feels rock tight and solid while letting me use the iPad effects as part of the live mix.

This setting applies to **insert returns**. The separate **Send FX** returns do not have an offset control in this build.
