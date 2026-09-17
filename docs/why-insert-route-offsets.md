# Why WitchboardX uses insert return offsets

The biggest reason for replacing per-track offsets was **CPU and reliability**. In my disting NT setup, earlier offset builds pushed WitchboardX toward 40% CPU and eventually caused audio cutouts; with Poly Res enabled, the whole NT could reach 99%. A mixer intended to be light on CPU was no longer usable for this patch. The earlier per-track design also gave every channel a timing control when the delay I needed to correct came from an external **insert** round trip.

The redesign sets timing per physical insert route. Tracks sharing an insert return share its offset, and the mixer only has to account for the routes in use. CPU improvements also came from skipping inactive Send FX work and refining the shared return path, so the improvement cannot honestly be credited to the offset control change alone. In the measured 44.1 kHz, ten-channel setup, the later build ran at **23% WitchboardX CPU with 0 ms offset** and **28% with an 18.2 ms insert offset**, with Poly Res enabled and no cutouts reported during that test.

The final **Offset** page now has **Bypass Offset**, **Insert route**, and **Insert return offset**. I select the insert route and set the measured round-trip delay once. **SC Lookahead** stays with the sidechain controls. WitchboardX uses the largest active insert offset as its timing reference and holds the faster paths back to meet the late return. The offset cannot make an external processor return audio earlier; it aligns the rest of the mix with it.

## Shared insert returns

Several channels can select the same physical insert route, for example the iPad route. WitchboardX detects this from the channels' active **insert route assignments**: it counts which channels feed each route, including routes involved in a live insert switch. It does **not** listen to the audio or compare bus numbers to guess whether two separately configured routes are the same device. To share a return, assign the channels to the same WitchboardX insert route.

WitchboardX sums those channels' outgoing audio onto that route's send bus. The iPad processes the combined signal and sends back **one** physical return. WitchboardX then reads and mixes that return **once per route**, even though several channels contributed to it. Reading it once avoids doubling the returned signal, and the route's single offset aligns that return with the dry mix. If contributors feed a Send FX path too, WitchboardX uses the **highest** contributor send amount for that shared return rather than adding the amounts together.

Once the external processor has combined those channels, its return is one audio signal; WitchboardX cannot split it back into separate channel returns. This feature is for deliberately shared **inserts**, distinct from the four **Send FX** return paths.

In my current patch, an insert bus goes out to an iPad and through a stack of effects before returning to WitchboardX. Setting **Insert return offset** to **6.0 ms** on that route brings the processed return into time with the dry paths. To my ears, the result is rock tight and solid while letting me use the iPad effects as part of the live mix. The 23%/28% figures above came from a separate 0 ms/18.2 ms comparison, not a CPU reading at 6.0 ms.

This setting applies to **insert returns**. The separate **Send FX** returns do not have an offset control in this build.
