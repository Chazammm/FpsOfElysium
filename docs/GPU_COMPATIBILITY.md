# AMD and NVIDIA compatibility audit

Scope: experimental 0.1.3, Windows x64, exact CoE5 5.43 executable, CoE5SE 3.2.2 API 11. Reviewed 2026-11-10.

**The current implementation does not require separate AMD and NVIDIA builds. Compatibility on NVIDIA is expected from the API requirements and source review, not yet confirmed by a hardware test.**

| GPU | Evidence | Status |
| --- | --- | --- |
| AMD Radeon RX 7900 XTX | Real-save FPS capture, native framebuffer/final-state oracle, actual GPU texture refresh/reuse/deletion and VAO-guard tests | Tested in the recorded scenes |
| NVIDIA Windows GPU exposing the required compatibility context/features | Source and standards review; NVIDIA documents OpenGL 4.6 support, for example on RTX 4090 | Expected compatible; no NVIDIA execution or FPS result |

## Requirements checked in the source

- OpenGL 3 or later with the legacy compatibility features used by CoE5. For 3.2 and later, the plugin checks the compatibility profile bit. A version number alone does not imply that legacy client arrays/QUADS are usable in a core-only context.
- GLSL 1.30 compatibility shaders with `sampler2DArray`, a flat integer layer varying and the standard `ftransform()` path. Shader compilation/linking and required entry points are checked at runtime.
- Standard texture-array allocation/upload and original RGBA8 mip-chain readback; actual maximum array-layer capacity is queried and the allocation is capped. Cache memory is limited to 128 MiB.
- Standard fixed-function client arrays and state save/restore. A bound VBO or nonzero VAO declines capture; supported states are checked before optimized drawing.
- Optional anisotropy, texture-swizzle and sampler-object checks depend on supported GL versions/extensions, not on a vendor name.

There are no AMD/ATI-only or NVIDIA-only entry points and no GPU-vendor-string branch in the production path. The game offsets refer to the pinned game executable, not to an AMD or NVIDIA driver binary. Unsupported features/states use original drawing; unsupported game executables receive no code patches.

## What the review does not establish

Different drivers can have different shader rounding, filtering results, client-array costs and submission behavior. AMD's recorded 1/255 image differences and performance gains cannot be assigned to NVIDIA without measurement. No vendor-specific workaround is justified by the current evidence.

A first NVIDIA validation should record the actual renderer/driver/context, verify `renderer_ready`, run framebuffer and final-state comparisons, exercise the GPU texture-refresh/reuse/deletion/VAO regressions, and perform a foreground FPS comparison at the same save/camera/zoom/quality. Only then can the status move from expected to tested.

## Primary references

- [Khronos OpenGL 3.0 specification](https://registry.khronos.org/OpenGL/specs/gl/glspec30.pdf): array textures are part of standard OpenGL.
- [Khronos GLSL 1.30 specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.1.30.pdf): shader types, texture sampling and interpolation used by the shader.
- [Khronos OpenGL 4.6 compatibility specification](https://registry.khronos.org/OpenGL/specs/gl/glspec46.compatibility.pdf): compatibility functionality and older GLSL language support.
- [NVIDIA RTX 4090 specifications](https://www.nvidia.com/en-eu/geforce/graphics-cards/40-series/rtx-4090/): an official NVIDIA example documenting OpenGL 4.6 support. This is not a test of this mod or proof for every NVIDIA model/context.
