# Changelog

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
