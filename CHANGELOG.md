# Changelog

## 0.1.10-local — experimental source snapshot, 2026-11-12

- Extend map-decoration batching to Low, Medium and High by wrapping their separate native branch and decoration call. Retain the existing Highest/Ultra path and use the same batching implementation for all five levels.
- Preserve each level's original quality value, object generation, draw order, source textures, mipmaps and filtering. The change does not substitute the Highest renderer for Low or alter the native sorting algorithms.
- Guard both added callsites in the existing all-before-write preflight and patch transaction: eight pinned game calls in total. Add original-branch forwarding tests and one-time diagnostics naming each quality and branch reached.
- Verify original/candidate framebuffer and final-state comparisons at every quality, original/original Low control, actual GPU cache/sampler regressions and repeated quality switching in one process. Differences stay within the existing isolated 1/255 rounding class; Low's recorded comparison was bit-exact.
- Add actual CoE5SE startup coverage across all five levels with precise pacing off/on and GPU wait off/on. Verify retained quality, explicit positive FPS limit, active pacing callback and timing-import ownership. No quality-dependent limiter behavior is added.
- Measure the new Low/Medium/High batching path against the previous unbatched path in the isolated native harness. Record section timings separately from whole-loop observations; no new foreground PresentMon result.
- Use consistent "Frames per second" option names without the "(approx.)" suffix in both timing modes. Keep the native timing explanation in tooltips and documentation; limiter behavior is unchanged by this naming update.

## 0.1.9-local — local test stage, included in the 0.1.10 snapshot

- Add the opt-in Graphics 2 checkbox "Precise frame rate limit", disabled by default, with immediate switching and INI persistence. Use the selected frame-rate target; keep native whole-millisecond behavior when disabled. Show the added numeric targets without "approx." while the precise mode is active.
- Pace the two pinned frame-loop paths with QPC and Windows high-resolution one-shot timers. Suppress only their native limiter sleeps after this pacer has taken responsibility; forward all other sleeps and return the original real GetTickCount values. No changes to animation time, rendering, VSync, hardware clocks, voltage or power limits.
- Cap background rendering at 30 FPS. Reset deadlines on focus/target/path changes and long pauses, skip missed deadlines and use bounded timer waits without spinning. Restore native waiting on timer failure.
- Before enabling, verify both timing imports and four indirect-call sites. Refuse conflicting timing hooks without overwriting them. Default-off startup leaves those two imports untouched.
- Add timing-guard, passthrough, target/parser and live-toggle/persistence regressions. Actual CoE5SE startup tests verify off/on import ownership, background pacing, preserved quality and explicit --maxfps priority.
- Investigate native coarse-clock waits and build an isolated laboratory comparison. The large scene removes about 1.5–1.7 ms of native waiting but still cannot reach 240 FPS; CPU time increases. A light scene reaches a mean interval around 4.167 ms with the refined pacer. These are instrumented loop measurements, not a new foreground PresentMon comparison. GPU watts and temperatures remain unmeasured.

## 0.1.8-local — experimental source snapshot, 2026-11-12

- Batch consecutive quads across up to eight texture-array pages without changing their order. Carry the page index separately from the original UV coordinates and layer index; preserve original mip chains, texture filtering and vertex colors.
- Keep the batching shader active across compatible flushes. Save, bind and restore all eight reserved texture-array units and their sampler objects for each drawing session. Rebind pages each session so foreign texture state is never assumed to persist.
- Read stable graphics state once per map pass and track the pinned game's relevant enable/disable, texture-environment and matrix changes. Keep native submission boundaries and original drawing for unsupported states and assets.
- Require nine fragment texture units for multi-page batching and sampler binding support when sampler objects exist; otherwise retain the previous single-page path. Reject explicit core-only contexts while accepting a zero profile mask.
- Add tracked-state regressions; verify multi-page bindings, foreign-state restoration across sessions, sampler overrides and fixed-function fallbacks on the actual GPU. Repeat original/candidate framebuffer and final-state comparisons at two zoom levels. The final run differed in two RGBA channels per image, at most 1/255, with matching checked state and no GL errors.
- Isolated Ultra map-section time: 6.02 ms versus 8.06–8.28 ms for the previous batching path, about 25–27% less section time. Roughly 4,580 draw submissions per frame become 420. No new whole-game foreground FPS result; no graphics-quality reduction or FPS-limiter change.
- Adapt ideas from musical_pianist's texture-array-batching contribution; retain the existing exact-game and loader guards. Verify the actual CoE5SE plugin reaches `renderer_ready` with both additions active.

## 0.1.7-local — local test stage, included in the 0.1.8 snapshot

- Remove Unlimited from Graphics 1. Keep the existing bounded 144/165/240/360 targets and the native 20/30/60/120 options.
- Migrate saved zero-delay preferences and legacy native zero-delay configurations to the existing bounded 360 target (3 ms). Repair a reloaded zero at the main-menu/map boundary too; do not change positive intervals or explicit positive launch limits.
- Map the legacy uncapped menu ID to the bounded target and never save zero through this menu.
- Test migration, repeated configuration reload, legacy selection, capped startup values, argument priority and the actual menu. Rendering and graphics quality are unchanged.

## 0.1.6-local — local test stage, included in the 0.1.8 snapshot

- Extend native Graphics 1 → Frame Rate with approximate 144/165/240/360 targets and Unlimited. Retain the original choices and native timing/selection handler; do not alter rendering, quality or VSync.
- Remember every native FPS-menu selection, including switching back to 60/120, in the mod INI. Restore it once after native configuration loading at the main menu or first map draw. Explicit `--maxfps=...` takes priority on restart.
- Mark higher targets as approximate and explain the corresponding native budgets: 7/6/4/3 ms. Unlimited uses 0 ms. These values do not guarantee a rendered frame rate or exact monitor synchronization.
- Guard the added radio-draw callsite in the existing complete preflight (six pinned game calls). Test native forwarding, all five added choices, persistence, invalid settings, legacy choices and one-time restore; verify startup values and the actual in-game menu.

## 0.1.5-local — local test stage, included in the 0.1.8 snapshot

- Add "Wait for GPU each frame" to the native Preferences → Graphics 2 page. Unchecked selects the game's existing `--noglfinish` behavior; checked selects `--glfinish`. It changes neither VSync nor graphics quality or the FPS limiter.
- Apply changes immediately and remember them in `FPSofElysiumSE.ini` beside the plugin, outside saves and the binary game configuration. Preserve the game's state when no preference exists; explicit launch flags take priority on restart.
- Guard both added native menu callsites in the existing all-before-write preflight and patch transaction (five game callsites in total).
- Verify native checkbox display, both toggle directions, persistence across restart, and argument/preference handling. No additional FPS improvement is claimed: the inspected CoE5 5.43 binary and the default lab launch already have this extra GPU wait disabled.

## 0.1.4-local — local test stage, included in the 0.1.8 snapshot

- Preflight all 48 GL/GLU import slots and the three pinned game callsites before changing graphics imports. A conflicting target disables the optimization and reports the function, slot and owner module.
- Log the local mod version and startup result; log vendor, renderer, OpenGL driver string, GLSL version, context profile, array-layer limit and cache limit once a current context is available.
- Record specific original-renderer fallback causes, emitting each category at most once per launch instead of an ambiguous generic ready/fallback message.
- Add tests for every modified import slot, altered callsite instructions/targets, diagnostic deduplication and unchanged main-menu callback forwarding.
- Rendering order, textures, mipmaps and shader output are unchanged. This local build has no new performance claim.

## 0.1.3 — experimental, 2026-11-10

- Release cached texture-array layers on source invalidation/deletion and reuse freed layers. Rehash deleted-name probe clusters so texture-name churn cannot permanently occupy all cache entries.
- Invalidate cached copies on the game's `gluBuild2DMipmaps` import, preserving the existing GLU forwarder chain. System GLU's internal uploads bypass the executable's `glTexImage2D` import hook.
- Synchronize logical 2D binding bookkeeping when the currently bound texture is deleted.
- Decline optimized capture with a nonzero vertex-array object bound.
- Add CPU regressions for 20,000 layer-update cycles, 20,000 deleted-name cycles, collision clusters and full-table deletion; add native GPU checks for GLU refresh, layer reuse, deleted names and the VAO guard.
- An original-versus-original image control was bit-exact. Two lab shader alternatives did not consistently remove the last 1/255 differences; the production shader remains unchanged.
- Verified CoE5SE 3.2.2 and the enabled mod's settings in the actual Steam installation. No new whole-game FPS benchmark.

## 0.1.2 — experimental, 2026-11-10

- Defer source 2D texture bindings during compatible texture-array capture and restore them at native/flush boundaries.
- Reuse client-array configuration across compatible batches; restore original client state before native state changes, fallback drawing and the end of the map pass.
- Add laboratory framebuffer and final drawing-state comparisons.
- Supply a portable build script that accepts separate compiler and CoE5SE source paths. Upstream source and compiled adapted loaders are not redistributed here.
- Instrumented map-section time decreased by about 37.5% versus the same code with both additions disabled. No new whole-game FPS comparison has been made.

## 0.1.1 — experimental, 2026-11-10

- Restore original immediate drawing for unsupported textures instead of producing tiny legacy client-array batches.

## 0.1.0 — experimental, 2026-11-10

- Initial CoE5SE API 11 plugin for exact Windows CoE5 5.43.
- Preserve original texture UVs, mip chains, object order and Highest/Ultra settings while batching compatible decorations with texture arrays.
- Match game hash/callsites before patching and refuse unsupported binaries.
- Integrate an explicit, exact-hash loader extension for private testing.
- Copied real-save foreground benchmark: 36.52 FPS disabled versus 64.00 FPS enabled at Ultra.
