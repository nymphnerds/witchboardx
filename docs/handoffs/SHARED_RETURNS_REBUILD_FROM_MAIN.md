# Shared insert return timing rebuild

**Update:** This document records an earlier candidate and contains obsolete CPU and object-SHA status. Continue from [the CPU refinement handoff](SHARED_RETURNS_CPU_REFINEMENT_HANDOFF.md) for the measured `cdab0dd` build.

## Branch and scope

- Worktree: `/home/nymph/DistingNT/WitchboardX-shared-returns`, branch `witchboardx-shared-returns`.
- Stable main remains in `/home/nymph/DistingNT/Witchboard-master` and was not edited.
- The prior per-channel negative offset system and Offset page are removed from this branch. The 241 parameter budget is preserved by reusing the two appended editor IDs for **Insert route** and **Insert return offset** on **Final Outputs**.
- SC Lookahead stays on **Sidechain/Master**. Bypass Offset stays on **Final Outputs** and retains the main baseline's auto-follow behavior.
- FX sends and FX returns keep their normal routing. There is no Send FX return offset control in this first version.

## Timing model

The highest configured offset among active physical insert routes sets the common reference. The aggregate Main dry path, aggregate Bypass dry path, and active sidechain key are delayed by that amount. A deferred insert return uses a route delay of `reference - route offset`. The route's physical return is read once, including when multiple channels share it. Its Send FX contribution uses the largest active contributor send amount. The delay rings are allocated at construction; there is no audio-loop allocation.

The current model treats each route setting as the effective latency of the final insert return. Serial Insert 1 + Insert 2 paths with different external latencies and a shared final route require device validation before relying on exact alignment.

## Measured host results

- Ten-channel DRAM requirement: **224,392 bytes** (host layout), versus **486,080 bytes** in the earlier tested shared-return/offset branch. The unchanged main baseline was about **347,424 bytes**.
- 24-frame host loop with one inserted channel and 20 ms compensation: **1.20 µs/block** for this branch versus **1.15 µs/block** for main in the same compiler/test setup. This is an approximate relative CPU check, not an NT meter reading.
- After an NT report that CPU was worse, a nine-active-channel host loop exposed the missed hot path: the first candidate took about **3.4 µs/block** with an active insert offset, versus **1.9 µs/block** for main. Restricting deferred-return processing to channels whose active paths actually reach a deferred route lowered the candidate to **2.2 µs/block** in that setup. Zero-offset processing stays about **1.9 µs/block**. Device CPU and sustained playback remain unverified.
- A second NT test reported **40% Witchboard CPU**, still above the user's low-20s target. Main dry, Bypass dry, and the SC key now use one five-signal ring because they always need the same insert-reference delay. Ten-channel host DRAM fell to **224,392 bytes**. The nine-channel host loop fell to about **2.1 µs/block**, versus **1.9 µs** for main. A loop loaded with the current preset's parameter values measured about **2.4 µs/block**, versus **2.1 µs** for main. These gains are too small to predict the NT percentage.
- The NT test sample rate was confirmed as **44.1 kHz**. Insert delay storage still reserves the 96 kHz maximum, but the active ring now spans 883 frames at 44.1 kHz instead of 1,921. A sample-rate change resets the ring state. This aims to reduce cache traffic on NT; the host preset benchmark remained about **2.4 µs/block** versus **2.1 µs/block** for main. Hardware CPU impact is unmeasured.
- All host routing, gain, sidechain, sends, timing, preset, and 32/44.1/48/96 kHz tests pass. ARM object inspection passes against the official API headers.
- The 40%-CPU object is preserved at `/tmp/WitchboardX-d95e953-route-cpu-fix.o` (SHA-256 `a068d87427d0c431370cba258c6fa37cb392f9af14df3f70983677858c13fde2`). The current candidate is `plugins/WitchboardX.o` (SHA-256 `1705a5da419f3946ad9e6aeed49c727ec839466c417cf18f3d966c2c7cc0df6c`).

## NT test gate

With the same preset and sample rate, compare stable main and this candidate with Poly Res enabled. First run one stereo iPad insert with route offset 0 then 20 ms; watch Witchboard CPU, overall CPU, and continuous audio for longer than the previous cutout interval. Then test two channels sharing one physical return, Send 1 at different levels, SC enabled, bypass output, and a live offset change. Host success does not establish that the NT cutout is fixed.
