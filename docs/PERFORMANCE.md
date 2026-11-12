# Performance and quality evidence

All measurements below are from one Windows system with an AMD Radeon RX 7900 XTX and CoE5 5.43. They are observations for specific scenes, not general FPS promises.

## Whole-game comparison: version 0.1.0

Two 15-second AMD PresentMon captures used a copy of the same real autosave, the game in the foreground, a requested 2560 × 1440 window and Ultra. Read-only process snapshots recorded exactly the same camera (53,27), zoom 12, plane 0, quality 13 and zoom speed 0. The save uses existing content mods; their files are not redistributed.

| Optimization | Frames | Mean frame time | Equivalent FPS | P95 frame time |
| --- | ---: | ---: | ---: | ---: |
| Off | 545 | 27.380 ms | 36.52 | 27.764 ms |
| On | 958 | 15.625 ms | 64.00 | 20.577 ms |

The gain is 75.23%. These were separate launches with animated scenery, not identical rendered inputs. This comparison measured plugin SHA-256 `1CE3A4BE42505A83DB1F37FA2A81A303E4261C1565C0EF25AA7329BD5A8557AD`. It has not been repeated with later versions. See `evidence/real-ultra-comparison.json`.

## Additional optimization: version 0.1.2

An isolated native GLU-forwarding harness measured the same map section without CoE5SE. It uses the production renderer headers but adds timing instrumentation. It fixes a synthetic test map to camera (40,40), plane 0, zoom 16, Ultra, a requested 1280 × 720 window and 16 process-local workers. The production mod does not fix camera, zoom or workers and does not include those timings.

The new code was compared with its two additional optimizations disabled and enabled. A steady 300-frame batch without them measured **12.56 ms** for the Highest/Ultra section. With deferred source bindings and reused client-array setup it measured **7.85 ms**, about **37.5% less section time**. Within that section, array submission fell from about 7.29 to 2.92 ms. Roughly 41,700 decorations remained; separate starts produced slightly different decoration counts. The enabled run also includes an occasional image/state oracle, so these are instrumented section timings, not overall FPS.

In the recorded 300-frame batches, source-texture binding submissions fell from about 41,170 to 8,940 per frame, and client-array setups from about 4,600 to 408 per frame. The counts describe the measured wrapper paths, not all driver-internal activity. Batches retain their original ordering.

## Image and state validation

The oracle restores the exact framebuffer background, then draws the original path and the candidate using the same prepared lists. It compares every RGBA byte and records OpenGL errors. Tests at zoom 16 and 32 with Ultra and zoom 64 with Highest exercise different map scales.

The Ultra zoom-16 check found 4, 1 and 1 differing channel values out of 3,686,400, each differing by only 1/255. Zoom 32 found 2, 3 and 0. Highest at zoom 64 found 2, 3 and 3. Both paths reported no OpenGL errors and the background restoration was exact. These are small rounding differences; the images are not perfectly bit-identical.

The new oracle also compares the final source/array texture bindings, program, client-active texture unit, vertex/color/texture-array formats, pointers and enabled states, and current color/texture coordinates. The checked states matched after each tested draw, and the retained client-array session was closed. This targeted state check does not prove compatibility with arbitrary native plugins or every OpenGL extension.

Unsupported textures and states use original immediate drawing. No texture resolution, mip level, decoration count, drawing order or quality setting is intentionally reduced. Longer gameplay, every plane, other GPUs and third-party native graphics modifications remain follow-up work.

## 0.1.3 correctness audit

The original renderer drawn twice against the restored background was bit-exact in all three control frames. Two laboratory color-varying shader alternatives still produced occasional one-unit channel differences, so the production shader was retained. The measured differences are isolated 1/255 changes, not missing details or reduced texture resolution; bit-exact equivalence across GPUs is not established.

The audit found that invalidation/deletion did not reclaim texture-array layers or deleted cache keys. It also found that the production plugin did not invalidate a cached texture when the game rebuilt its mipmaps through GLU, whose uploads bypass the game's OpenGL import slot. Version 0.1.3 fixes these cases and refuses capture with a nonzero VAO bound. The CPU regression covers 20,000 updates, 20,000 deleted-name cycles, wrapped collisions and a full table. A GPU test uploads known pixels, caches them, rebuilds the same texture through the production GLU wrapper, verifies the new pixels and reused layer, deletes the texture and checks removal, and exercises the nonzero-VAO guard. The native forwarder has no separate invalidation path that could mask a broken production wrapper.

These checks and the framebuffer/final-state oracle passed. This correctness release has no new whole-game FPS measurement.

## Multi-page batching and state tracking: 0.1.8-local

The incoming texture-array-batching contribution from musical_pianist supplied useful multi-page and state-tracking ideas. The local implementation adapts them to the existing plugin. It combines up to eight array pages in one shader without sorting or moving any decorations. Units 1–8 are explicitly saved, rebound each drawing session and restored, including sampler objects; unit 0 retains the original source texture. Relevant native enable/disable, texture-environment and matrix operations update or invalidate the state cache. Native drawing boundaries still flush buffered geometry and close the shader/client-array session.

The native harness used the same fixed scene setup as the earlier section experiment: Ultra (13), plane 0, camera (40,40), zoom 16, requested 1280 × 720, 16 process-local workers and a bounded 240-FPS target. Each launch lasted 20 seconds. The first 300-frame timing block was excluded for cache warmup. Timing runs had framebuffer oracles and GPU fixtures disabled. Each number below is the mean of the remaining 300-frame section means, with three or four steady blocks per run. See `evidence/multipage-comparison-0.1.8.json` and its named logs.

| Multi-page | State tracking | Highest/Ultra section mean |
| --- | --- | ---: |
| Off | Off | 8.064 ms |
| On | Off | 6.305 ms |
| Off | On | 7.917 ms |
| On | On | 6.024 ms |
| Off | Off, repeated after the candidate | 8.285 ms |

Both additions together reduced the measured section by 25.3–27.3% against the two old-path runs. The single-feature runs are diagnostic samples, not repeated estimates of each feature's independent gain. Draw submissions fell from roughly 4,580 to 420 per frame, and stable state-check groups from about 250 to one per frame. Decorations remained roughly 41,800–41,900, with small differences between separately launched animated scenes. These are wrapper counters and CPU-observed section times, not GPU execution times or whole-game frame rates. They do not establish an equivalent FPS increase.

Separate quality runs compared the original path against the candidate using identical prepared lists and a restored framebuffer, at zoom 16 and 12. Across these runs, zero to two RGBA channels per image differed out of 3,686,400, always by at most 1/255. The final run reported two channels in each of its three comparisons. An original-versus-original control at zoom 12 was bit-exact. All recorded comparisons had exact background restoration, zero GL errors, equal checked final state and a closed client-array session. The extended state oracle includes active texture unit and array/sampler bindings on units 0–8. This is the same tiny rounding class as earlier versions, not bit-exact equivalence.

Actual GPU fixtures verified two distinct array-page groups, binding the right pages, temporarily neutralizing foreign sampler objects and restoring foreign array/sampler bindings across two successive sessions. Fog, a non-modulating texture environment and a nonidentity texture matrix triggered original-path refusal and recovered after restoration. Existing texture-refresh, layer-reuse, deletion and VAO regressions also passed. The actual CoE5SE 3.2.2 plugin then reached `renderer_ready` as 0.1.8-local with `multi_page=1 state_tracking=1` in the isolated profile.

No new foreground whole-game FPS capture was completed. Long sessions, every plane, arbitrary native graphics hooks and NVIDIA execution remain unverified for this update. The FPS-limit and graphics-quality settings are unchanged by 0.1.8.
