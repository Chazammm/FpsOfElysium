# Performance and quality evidence

All measurements below are from one Windows system with an AMD Radeon RX 7900 XTX and CoE5 5.43. They are observations for specific scenes, not general FPS promises.

## Whole-game comparison: version 0.1.0

Two 15-second AMD PresentMon captures used a copy of the same real autosave, the game in the foreground, a requested 2560 × 1440 window and Ultra. Read-only process snapshots recorded exactly the same camera (53,27), zoom 12, plane 0, quality 13 and zoom speed 0. The save uses existing content mods; their files are not redistributed.

| Optimization | Frames | Mean frame time | Equivalent FPS | P95 frame time |
| --- | ---: | ---: | ---: | ---: |
| Off | 545 | 27.380 ms | 36.52 | 27.764 ms |
| On | 958 | 15.625 ms | 64.00 | 20.577 ms |

The gain is 75.23%. These were separate launches with animated scenery, not identical rendered inputs. This comparison measured plugin SHA-256 `1CE3A4BE42505A83DB1F37FA2A81A303E4261C1565C0EF25AA7329BD5A8557AD`. It has not been repeated with 0.1.2. See `evidence/real-ultra-comparison.json`.

## Additional optimization: version 0.1.2

An isolated native GLU-forwarding harness measured the same map section without CoE5SE. It uses the production renderer headers but adds timing instrumentation. It fixes a synthetic test map to camera (40,40), plane 0, zoom 16, Ultra, a requested 1280 × 720 window and 16 process-local workers. The production mod does not fix camera, zoom or workers and does not include those timings.

The new code was compared with its two additional optimizations disabled and enabled. A steady 300-frame batch without them measured **12.56 ms** for the Highest/Ultra section. With deferred source bindings and reused client-array setup it measured **7.85 ms**, about **37.5% less section time**. Within that section, array submission fell from about 7.29 to 2.92 ms. Roughly 41,700 decorations remained; separate starts produced slightly different decoration counts. The enabled run also includes an occasional image/state oracle, so these are instrumented section timings, not overall FPS.

In the recorded 300-frame batches, source-texture binding submissions fell from about 41,170 to 8,940 per frame, and client-array setups from about 4,600 to 408 per frame. The counts describe the measured wrapper paths, not all driver-internal activity. Batches retain their original ordering.

## Image and state validation

The oracle restores the exact framebuffer background, then draws the original path and the candidate using the same prepared lists. It compares every RGBA byte and records OpenGL errors. Tests at zoom 16 and 32 with Ultra and zoom 64 with Highest exercise different map scales.

The Ultra zoom-16 check found 4, 1 and 1 differing channel values out of 3,686,400, each differing by only 1/255. Zoom 32 found 2, 3 and 0. Highest at zoom 64 found 2, 3 and 3. Both paths reported no OpenGL errors and the background restoration was exact. These are small rounding differences; the images are not perfectly bit-identical.

The new oracle also compares the final source/array texture bindings, program, client-active texture unit, vertex/color/texture-array formats, pointers and enabled states, and current color/texture coordinates. The checked states matched after each tested draw, and the retained client-array session was closed. This targeted state check does not prove compatibility with arbitrary native plugins or every OpenGL extension.

Unsupported textures and states use original immediate drawing. No texture resolution, mip level, decoration count, drawing order or quality setting is intentionally reduced. Longer gameplay, every plane, other GPUs and third-party native graphics modifications remain follow-up work.
