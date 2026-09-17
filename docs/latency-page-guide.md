# Latency page guide

The **Latency** page groups the controls that keep WitchboardX's audio paths in time. It covers sidechain lookahead, the Bypass path, and external **insert** returns. The four **Send FX** returns have no separate offset control.

| Control | Purpose |
| --- | --- |
| **SC Lookahead** | Delays Main audio when Sidechain is on. It is the **same parameter** shown on Sidechain/Master, so changing it on either page changes one setting. |
| **Bypass Offset** | Delays Bypass audio. A change to SC Lookahead copies the active lookahead value here to keep Main and Bypass aligned. You can set a different Bypass delay manually afterward. |
| **Insert route** | Selects the physical insert route, A–F, whose offset you want to edit. |
| **Insert return offset** | Describes that route's external round-trip delay, from 0.0 to 20.0 ms. Each route stores its own value. |

## Independent insert route offsets

Set each insert route to the round-trip delay of the external processor connected to it. **The routes have independent offset settings.** WitchboardX then uses the largest offset among *active* routes as a common timing reference and delays faster paths by only the difference they need.

For example, if route A is set to **10 ms** and route B to **6 ms**:

| Path | Extra delay inside WitchboardX to reach the insert reference |
| --- | ---: |
| Route A return | 0 ms |
| Route B return | 4 ms (`10 − 6`) |
| Dry audio | 10 ms |

Route B is **not** delayed by 6 ms *plus* 10 ms. Its external round trip accounts for the first 6 ms; WitchboardX adds the remaining 4 ms. If route A becomes inactive, the reference can fall to 6 ms. With one active insert route, that route's return normally needs no extra delay; the dry paths wait for it. An offset cannot make an external return arrive earlier.

Several channels may use the **same** insert route. They share that route's offset, and WitchboardX mixes its one physical return once. [Shared insert returns and timing](why-insert-route-offsets.md) explains why this matters.

## SC Lookahead and Bypass Offset

SC Lookahead is separate from the insert route settings. When Sidechain is on, it delays Main audio before ducking. Changing SC Lookahead to **6.0 ms** copies **6.0 ms** to Bypass Offset, keeping the Main and Bypass paths aligned after insert compensation.

You can then set Bypass Offset to an absolute value, such as **10.0 ms**, if a different Bypass delay is required. That manual value holds until the next SC Lookahead or Sidechain mode change, which copies the active lookahead value again. When Sidechain is off, the automatic Bypass value is **0.0 ms**.
