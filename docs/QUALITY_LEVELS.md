# Graphics-quality coverage

Version 0.1.10 extends decoration batching to all five map-quality settings. Earlier builds optimized only Highest/Ultra. The separate native Low/Medium/High branch still used immediate decoration drawing, which could make a lower setting slower than optimized Ultra.

| Graphics setting | Native value | Native map branch | Batching in 0.1.10 |
| --- | ---: | --- | --- |
| Low | 9 | Low/Medium/High | Enabled |
| Medium | 10 | Low/Medium/High | Enabled |
| High | 11 | Low/Medium/High | Enabled |
| Highest | 12 | Highest/Ultra | Enabled |
| Ultra | 13 | Highest/Ultra | Enabled |

Both native branches call the same guarded batching implementation. Each level still uses its own original object generation, sorting and drawing order. Only compatible consecutive decoration quads are buffered; unsupported textures/states use original drawing. The mod does not change the quality value, promote Low to Ultra or remove details from the selected level. The exact EXE guard and eight-callsite preflight cover both branches and their decoration calls before patches are applied.

The FPS menu, optional precise limiter, background cap and GPU-wait setting are independent of graphics quality. Their original implementation is retained; this update adds coverage and verification rather than a separate limiter for each quality.

## Verification

The isolated native test uses the same production rendering headers, Ultra/Low/etc. as explicitly selected, camera (40,40), plane 0, zoom 12 and a requested 1280 × 720 window. Original/candidate comparisons restore the framebuffer background and compare the same scene's two drawing paths within one frame. Three comparisons per quality found:

| Quality | Differing RGBA channel values per image, out of 3,686,400 | Maximum difference |
| --- | --- | --- |
| Low | 0, 0, 0 | 0 |
| Medium | 1, 1, 1 | 1/255 |
| High | 0, 2, 0 | 1/255 |
| Highest | 3, 0, 0 | 1/255 |
| Ultra | 0, 0, 0 | 0 |

All checked final graphics states matched, restored backgrounds were exact and both paths reported zero OpenGL errors. The original/original Low control was bit-exact. Actual GPU refresh/reuse/deletion, VAO refusal, multi-page binding and foreign-sampler restoration fixtures passed at every level.

A single-process stress run switched through 9→10→11→12→13→9→13→11→10→9, comparing original/candidate images throughout. Its recorded zoom-12 run checked 18 images, covered every level and found at most one channel differing by 1/255, with matching state and no GL errors. Cache memory stayed at the same allocation after warmup; no additional rejected texture appeared during the switches. These are targeted scene tests, not proof for every plane, GPU or native plugin combination.

The same switching test at zoom 64 added 15 comparisons covering all five levels. They also retained matching state, exact background restoration and zero GL errors; at most three channel values differed per image, each by 1/255.

Actual CoE5SE startup tests cover each quality with precise mode off / GPU wait off, precise mode on / GPU wait off, and precise mode on / GPU wait on. The tests inspect the running process to verify that the selected quality remains intact, an explicit --maxfps=60 retains its native 16-ms interval, and the timing imports belong to the precision module only while enabled. The enabled pacing callback must run at every level. Native and optimized branch forwarding and all eight callsite guards are also checked by the build tests.

## Performance and limits

Timing runs use the same isolated scene setup and a bounded 240 target. First 300-frame batches are excluded for warmup. Longer unbatched baseline runs are used because the lower native path can take too long to produce enough steady samples in a short launch. See `evidence/all-quality-0.1.10.json` for named logs, sample counts and section means.

| Quality | Previous unbatched section | Extended batching section | Steady 300-frame blocks, old/new |
| --- | ---: | ---: | ---: |
| Low | 8.670 ms | 1.619 ms | 6 / 6 |
| Medium | 15.985 ms | 2.659 ms | 4 / 5 |
| High | 17.421 ms | 3.490 ms | 3 / 5 |

These are section-time observations, not foreground FPS promises. Unbatched launches lasted 40 seconds and optimized launches 15 seconds. The benchmark retains each level's original scene detail; separate launches have small animation/content differences. Other map/UI work and driver scheduling remain. The new branch coverage fixes the known optimization gap; it does not guarantee strictly ordered FPS for every quality choice and scene.

The user's 140–150 FPS report with precise mode is a useful real-play observation for 0.1.9, not a controlled comparison for 0.1.10. No new foreground PresentMon benchmark has been recorded for this update. Hardware power/temperature and NVIDIA execution remain unmeasured.
