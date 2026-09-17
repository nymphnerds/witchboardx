# The Latency page

The **Latency** page brings four timing controls together. It helps keep the Main and Bypass paths aligned and lets an external insert return line up with the rest of the mix.

| Control | What it does |
| --- | --- |
| **SC Lookahead** | Delays Main audio for sidechain ducking when Sidechain is on. This is the **same setting** shown on the Sidechain/Master page. |
| **Bypass Offset** | Delays Bypass audio. Changing SC Lookahead normally copies its active delay here so Bypass and Main stay aligned. You can adjust Bypass Offset manually afterward. |
| **Insert route** | Chooses which physical insert route, A–F, you are editing. |
| **Insert return offset** | Sets that route's external round-trip delay, from 0.0 to 20.0 ms. Channels sharing the route share this setting. |

## Main and Bypass

Set SC Lookahead to **6.0 ms** with Sidechain on. WitchboardX sets Bypass Offset to **6.0 ms** as well. That keeps audio taking the Bypass path in time with Main. If you then set Bypass Offset to **10.0 ms**, it stays at 10.0 ms as a manual override. The next change to SC Lookahead copies the new active lookahead value to Bypass Offset again. With Sidechain off, the automatic value is 0.0 ms.

The SC Lookahead controls on the two pages are one NT parameter, not two independent lookaheads. Bypass Offset is a separate delay for the Bypass audio path.

## External inserts

If an iPad or computer returns an insert about **6.0 ms** late, select its **Insert route** and set **Insert return offset** to **6.0 ms**. This is independent of SC Lookahead and Bypass Offset. WitchboardX uses the largest offset among the active insert routes as the common reference and holds faster paths back to match. It cannot make the external return arrive earlier.

If several channels use the same insert route, their audio is combined on its send and its physical return is mixed **once**. Select the same route on each channel so they share both the return and its offset. [Shared insert returns and timing](why-insert-route-offsets.md) explains this in more detail.

The four **Send FX** returns have no independent offset control in this build.
