# Why WitchboardX uses insert return offsets

The biggest reason for replacing per-track offsets was **CPU and reliability**. In my disting NT setup, earlier offset builds pushed WitchboardX toward 40% CPU and eventually caused audio cutouts; with Poly Res enabled, the whole NT could reach 99%. A mixer intended to be light on CPU was no longer usable for this patch. The earlier per-track design also gave every channel a timing control when the delay I needed to correct came from an external **insert** round trip.

The redesign sets timing per physical insert route. Tracks sharing an insert return share its offset, and the mixer only has to account for the routes in use. CPU improvements also came from skipping inactive Send FX work and refining the shared return path, so the improvement cannot honestly be credited to the offset control change alone. In the measured 44.1 kHz, ten-channel setup, the later build ran at **23% WitchboardX CPU with 0 ms offset** and **28% with an 18.2 ms insert offset**, with Poly Res enabled and no cutouts reported during that test.

The final **Offset** page now has **Bypass Offset**, **Insert route**, and **Insert return offset**. I select the insert route and set the measured round-trip delay once. **SC Lookahead** stays with the sidechain controls. WitchboardX uses the largest active insert offset as its timing reference and holds the faster paths back to meet the late return. The offset cannot make an external processor return audio earlier; it aligns the rest of the mix with it.

In my current patch, an insert bus goes out to an iPad and through a stack of effects before returning to WitchboardX. Setting **Insert return offset** to **6.0 ms** on that route brings the processed return into time with the dry paths. To my ears, the result is rock tight and solid while letting me use the iPad effects as part of the live mix. The 23%/28% figures above came from a separate 0 ms/18.2 ms comparison, not a CPU reading at 6.0 ms.

This setting applies to **insert returns**. The separate **Send FX** returns do not have an offset control in this build.
