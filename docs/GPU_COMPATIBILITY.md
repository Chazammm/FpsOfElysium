# AMD and NVIDIA compatibility audit

Scope: local 0.1.10, Windows x64, exact CoE5 5.43 executable, CoE5SE 3.2.2 API 11. Updated 2026-11-12. The primary references below were used in the original 0.1.3 audit; subsequent additions were checked in source and on the local AMD GPU.

**The current implementation does not require separate AMD and NVIDIA builds. Compatibility on NVIDIA is expected from the API requirements and source review, not yet confirmed by a hardware test.**

| GPU | Evidence | Status |
| --- | --- | --- |
| AMD Radeon RX 7900 XTX | Earlier real-save FPS capture; 0.1.8 section timings, framebuffer/final-state oracle, GPU texture refresh/reuse/deletion/VAO tests and multi-page/sampler-state restoration | Tested in the recorded scenes; no new whole-game FPS capture |
| NVIDIA Windows GPU exposing the required compatibility context/features | Source and standards review; NVIDIA documents OpenGL 4.6 support, for example on RTX 4090 | Expected compatible; no NVIDIA execution or FPS result |

## Requirements checked in the source

- OpenGL 3 or later with the legacy compatibility features used by CoE5. For 3.2 and later, the plugin rejects an explicit core-only profile mask. A zero profile mask is accepted; this alone does not establish support, so the other capability checks still apply. A version number alone does not imply that legacy client arrays/QUADS are usable in a core-only context.
- GLSL 1.30 compatibility shaders with `sampler2DArray`, a flat integer layer varying and the standard `ftransform()` path. Shader compilation/linking and required entry points are checked at runtime.
- Standard texture-array allocation/upload and original RGBA8 mip-chain readback; actual maximum array-layer capacity is queried and the allocation is capped. Cache memory is limited to 128 MiB.
- Standard fixed-function client arrays and state save/restore. A bound VBO or nonzero VAO declines capture; supported states are checked before optimized drawing.
- Optional anisotropy, texture-swizzle and sampler-object checks depend on supported GL versions/extensions, not on a vendor name.
- Multi-page batching requires at least nine fragment texture units. Units 1–8 hold the eight pages; unit 0 retains source-texture behavior. If sampler objects exist, `glBindSampler` must also be available. Without these additions the renderer uses the previous single-page path.
- Each multi-page session saves and restores the reserved array bindings and sampler objects. Foreign nonzero samplers on units 1–8 are temporarily unbound so their filters cannot replace the page's original filtering. Unit 0's existing sampler guard still applies.

There are no AMD/ATI-only or NVIDIA-only entry points and no GPU-vendor-string branch in the production path. The game offsets refer to the pinned game executable, not to an AMD or NVIDIA driver binary. Unsupported features/states use original drawing; unsupported game executables receive no code patches.

The optional 0.1.9 frame limiter uses Windows QPC and a high-resolution waitable timer, independently of the GPU vendor. The high-resolution timer flag requires Windows 10 version 1803 or later; failure leaves native waiting active. It changes no driver power policy or hardware tuning. See [frame-pacing evidence and power limits](FRAME_PACING.md). This adds no NVIDIA hardware validation.

Version 0.1.10 extends the existing batcher to both native map-quality branches. The local AMD validation covers all five quality levels, repeated quality switching and the graphics controls at every level. It introduces no new OpenGL feature or vendor-specific route. See [quality coverage](QUALITY_LEVELS.md); NVIDIA execution remains unverified.

## What the review does not establish

Different drivers can have different shader rounding, filtering results, client-array costs and submission behavior. AMD's recorded 1/255 image differences and performance gains cannot be assigned to NVIDIA without measurement. No vendor-specific workaround is justified by the current evidence.

A first NVIDIA validation should record the actual renderer/driver/context, verify `renderer_ready`, run framebuffer and final-state comparisons, exercise the GPU texture-refresh/reuse/deletion/VAO regressions, and perform a foreground FPS comparison at the same save/camera/zoom/quality. Only then can the status move from expected to tested.

For 0.1.8 this must also exercise multi-page rendering, foreign sampler/array bindings across consecutive sessions and the tracked-state fallbacks. The incoming contribution reported no gain on its NVIDIA test, which is not a result for this adapted plugin. No NVIDIA speedup is promised, and an AMD section-time gain is not a substitute for a NVIDIA test.

## Primary references

- [Khronos OpenGL 3.0 specification](https://registry.khronos.org/OpenGL/specs/gl/glspec30.pdf): array textures are part of standard OpenGL.
- [Khronos GLSL 1.30 specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.30.pdf): shader types, texture sampling and interpolation used by the shader.
- [Khronos OpenGL 4.6 compatibility specification](https://registry.khronos.org/OpenGL/specs/gl/glspec46.compatibility.pdf): compatibility functionality and older GLSL language support.
- [NVIDIA RTX 4090 specifications](https://www.nvidia.com/en-eu/geforce/graphics-cards/40-series/rtx-4090/): an official NVIDIA example documenting OpenGL 4.6 support. This is not a test of this mod or proof for every NVIDIA model/context.
