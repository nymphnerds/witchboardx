# Shared insert returns rebuilt from stable main

## Source and scope

- Branch/worktree: `witchboardx-shared-returns-rebuild` at `/home/nymph/DistingNT/WitchboardX-shared-returns-rebuild` in `NymphsCore_Lite`.
- Base: clean `Witchboard-master/main` commit `4eb5eae`. Main and the earlier cutout worktree were not changed by this rebuild.
- The previous shared returns implementation was not copied. This branch adds route ownership around the main branch's existing inserts and delays.
- Existing NT parameter IDs, preset layout, sidechain envelope, channel delay, Main delay, and Bypass delay behavior remain as on main.

## Signal and timing dependencies

1. Each enabled channel sends its normal gained signal through Insert 1 and, if selected, Insert 2. Repeat Protection determines the effective final route.
2. One route used as the final insert by multiple channels returns one already-summed signal. The plugin reads and mixes it to Main once, with no extra dry return copy or contributor-count normalization.
3. Existing Send 1/Radiant levels remain per-channel. The single shared return feeds Radiant once at the highest active contributor's Send 1 wet gain, without reducing Main.
4. Existing global offset base and Sidechain key alignment remain on main's timing path. A shared return has its own delay line after the physical return. Its most negative contributor offset wins; no contributor's channel delay is applied to that shared return.
5. Insert fades may have multiple active paths. Route-use metadata includes all potentially active final paths, while the audio loop marks which shared paths actually contributed in each frame.

## Memory and CPU scope

Six stereo route delay rings are allocated in NT DRAM at construction. Ten-channel host DRAM requirement is 486,080 bytes, versus about 347 KiB in the previous worktree. There is no allocation in `step()`. Route delay processing runs only when a route is shared and carries a return. Ordinary single-channel insert paths retain main's delay implementation. NT CPU load and cutout behavior remain unmeasured.

## Verification and hardware gate

- `make verify` passed: gain, sidechain, channels, sends, offsets, shared return level and fade tests at 32, 44.1, 48, and 96 kHz; preset migrations; ARM object inspection.
- Candidate `plugins/WitchboardX.o` SHA-256: `5eebc4a5adefee63cef58cac93e4c629c8dcadfc433917c080f05b79a8db4562`.
- Do not treat host success as proof of a cutout fix. On NT, first confirm the stable main object with the same patch, then load this candidate and test: one neutral insert; one stereo iPad insert with zero and negative offsets; two channels sharing a route with Send 1 zero/nonzero; mixed contributor offsets; and Sidechain enabled. Watch for cutouts after several minutes and during insert/offset changes.
