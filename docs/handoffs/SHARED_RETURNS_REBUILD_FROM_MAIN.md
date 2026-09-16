# Shared insert return timing rebuild

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

- Ten-channel DRAM requirement: **232,072 bytes** (host layout), versus **486,080 bytes** in the earlier tested shared-return/offset branch. The unchanged main baseline was about **347,424 bytes**.
- 24-frame host loop with one inserted channel and 20 ms compensation: **1.20 µs/block** for this branch versus **1.15 µs/block** for main in the same compiler/test setup. This is an approximate relative CPU check, not an NT meter reading.
- After an NT report that CPU was worse, a nine-active-channel host loop exposed the missed hot path: the first candidate took about **3.4 µs/block** with an active insert offset, versus **1.9 µs/block** for main. Restricting deferred-return processing to channels whose active paths actually reach a deferred route lowered the candidate to **2.2 µs/block** in that setup. Zero-offset processing stays about **1.9 µs/block**. Device CPU and sustained playback remain unverified.
- All host routing, gain, sidechain, sends, timing, preset, and 32/44.1/48/96 kHz tests pass. ARM object inspection passes against the official API headers.
- The earlier tested object was preserved at `/tmp/WitchboardX-shared-returns-tested-a748d29.o` (SHA-256 `490251af77208554e79fceb31bd07964580dbef900a2edbba8aba018f6557399`). The pre-CPU-fix object is `/tmp/WitchboardX-984fd7b-pre-cpu-fix.o` (SHA-256 `2af21e7f44694db435857396f25d6cdd932db46bff8e5a576af3b7e84cec228e`). The current candidate is `plugins/WitchboardX.o` (SHA-256 `a068d87427d0c431370cba258c6fa37cb392f9af14df3f70983677858c13fde2`).

## NT test gate

With the same preset and sample rate, compare stable main and this candidate with Poly Res enabled. First run one stereo iPad insert with route offset 0 then 20 ms; watch Witchboard CPU, overall CPU, and continuous audio for longer than the previous cutout interval. Then test two channels sharing one physical return, Send 1 at different levels, SC enabled, bypass output, and a live offset change. Host success does not establish that the NT cutout is fixed.
