# FpsOfElysium

Experimental development snapshot **0.1.8-local** for **Conquest of Elysium 5.43, Windows x64**, using **CoE5SE 3.2.2 / API 11**. It reduces the overhead of drawing large, zoomed-out maps while retaining the selected Highest/Ultra quality. This repository contains the tested source snapshot; the build identifier retains its local-test suffix.

The original map path submits tens of thousands of small decoration draws. This mod batches compatible consecutive decorations using texture arrays, preserves their original texture coordinates and complete existing mip chains, defers redundant source-texture bindings, and reuses client-array setup between compatible batches. It preserves object order and uses the original path for unsupported textures and states. It changes no gameplay, camera controls, worker count or quality setting.

**This is an experimental native plugin with an explicit loader adaptation. Stock CoE5SE 3.2.2 does not load external native plugins.** The adaptation admits only the exact locally built FPSofElysiumSE DLL hash and retains the loader's normal approval process. This is not an official CoE5SE release or a Workshop-only mod.

## Results and quality checks

On an RX 7900 XTX, the copied real-save benchmark at Ultra improved from **36.5 FPS off to 64.0 FPS on (+75.2%)**. That full FPS measurement used version 0.1.0; it has not been repeated for later versions. Both 15-second captures used a requested 2560 × 1440 window and the same read-back plane, zoom and camera. Separate launches mean the animated scenes were not pixel-identical.

Version 0.1.2 additionally reduces repeated texture binding and client-array setup. Version 0.1.3 fixes texture-cache lifetime and GLU-update handling and adds a VAO compatibility guard. See [measurement details](docs/PERFORMANCE.md) for section timings and their limits. Image comparisons retain all details and have shown only a handful of RGBA channel differences, at most 1/255. The images are not perfectly bit-identical. These are targeted tests, not a guarantee for every GPU, plane or mod combination. The full FPS benchmark has not been repeated for these updates.

Version 0.1.8 combines consecutive decorations from up to eight texture-array pages and tracks the game's relevant graphics-state changes. In the isolated Ultra test, the measured map section took **6.02 ms instead of 8.06–8.28 ms**, about **25–27% less time**. Recorded draw submissions fell from roughly 4,580 to 420 per frame. This is a section-time result against the previous batching path, not a new whole-game FPS measurement. Original UVs, mipmaps, filtering and object order are retained. Reserved texture units and sampler bindings are restored before returning to native drawing.

## Build

Dependencies are obtained separately: the user's own CoE5 5.43 installation, the complete CoE5SE 3.2.2 installation including its source directory, and llvm-mingw for Windows x64. The development compiler was llvm-mingw 20260922 UCRT x86_64.

```powershell
.\scripts\Build.ps1 -Compiler 'C:\tools\llvm-mingw\bin\x86_64-w64-mingw32-clang.exe' -CoE5SESource 'C:\tools\CoE5SE\src'
```

This builds the plugin and a matching adapted loader in `build`, checks texture-cache regressions, then checks null/API-version and foreign-executable guards. The loader is built from the separately supplied CoE5SE source using its `LOADER_EXTRA_HEADER` extension. Upstream loader/SDK files and compiled loader binaries are not redistributed in this repository or its release assets. Rebuilding the plugin requires rebuilding its matching loader. The release ZIP contains the mod plugin and metadata; the matching adapted loader must be built separately using this script and the separate upstream source.

## Test installation and removal

1. Close the game. Use a separate game/profile copy with the full CoE5SE 3.2.2 already installed according to its own instructions. Back up that test profile before changing it.
2. Back up the test profile's `mods/CoE5SE/coe5se_loader.dll`, then replace it with your matching `build/coe5se_loader.dll`. Keep the remaining CoE5SE files from the same 3.2.2 installation.
3. Copy `mod/FPSofElysiumSE` to the same profile's local `mods` directory and add `build/FPSofElysiumSE.c5plugin.dll` inside it. Enable CoE5SE and FPSofElysiumSE and follow the normal plugin approval prompt.
4. The “Smoother map rendering” setting defaults to on. Its startup on/off paths and the main-menu UI toggle/reset are verified. Mid-map live switching is not comprehensively tested.

To remove it, close the game, disable FPSofElysiumSE, and restore the backed-up original loader. Disabling its setting uses the original Highest/Ultra drawing path while the plugin remains loaded.

The exact supported EXE SHA-256 is `489d0b82fa790ec36e69a23e2335d0c2048fa57f4051fe8159c64f2d33508e55`. A different executable receives no code patches. Texture-array caching is limited to 128 MiB. Unsupported contexts use the original path. Long sessions, all planes, multiplayer and arbitrary other native graphics plugins are not comprehensively tested. Normal game data/content mods are not included.

For startup problems inspect `coe5mod.log` beside the launched executable. A successful load reports `fps_mod initialized` and, once the map is drawn, `fps_mod renderer_ready`. A loader and plugin hash mismatch requires rebuilding both as a matching pair. A game hash/signature mismatch leaves the optimization disabled.

The local update logs its version, checks the 48 GL/GLU import slots before replacing them, and identifies a conflicting function/module without overwriting the foreign import. GPU/OpenGL driver information is logged once when a context is available; each original-renderer fallback category is logged at most once. See [diagnostic messages](docs/DIAGNOSTICS.md). This detects changed import targets and the six pinned game callsites, not arbitrary inline detours or modifications installed later.

Version 0.1.5 adds **Preferences → Graphics 2 → Wait for GPU each frame**. Leave it unchecked for the game's `--noglfinish` behavior; checking it enables the extra `glFinish` wait. The change applies immediately and is remembered in `FPSofElysiumSE.ini` beside the plugin. Explicit `--glfinish`/`--noglfinish` launch arguments take priority on the next start. Without a saved preference, the existing game state is preserved. This is independent of VSync, the FPS limit and the "Smoother map rendering" toggle, and changes no graphics quality. The tested game's default is already unchecked, so this option alone has no demonstrated additional FPS gain.

The local build offers **144, 165, 240 and 360 FPS targets** under **Preferences → Graphics 1 → Frame Rate**, alongside the original 20/30/60/120 options. Version 0.1.7 removes Unlimited to avoid unnecessary GPU load. An old saved unlimited choice or a native zero-delay configuration is converted to the existing bounded 360 target (3 ms); other selections are preserved. The targets are approximate because the game's native limiter uses whole milliseconds. The new intervals are respectively 7/6/4/3 ms (nominal arithmetic rates about 143/167/250/333 FPS); these are not measured FPS or precise caps. Rendering, scheduling, VSync and driver limits can reduce actual FPS. A higher setting does not make an overloaded scene render faster. Choices apply immediately and are remembered in the same mod INI. Explicit positive `--maxfps=...` arguments take priority at restart. The saved interval is restored once after configuration loading; subsequent bounded menu changes take effect normally. A reloaded zero-delay configuration is also repaired. No new frame scheduler, graphics-quality change or performance claim is introduced.

AMD/NVIDIA: the code uses standard OpenGL capabilities and needs no separate vendor build. AMD RX 7900 XTX has been tested; NVIDIA compatibility is expected but not yet hardware-verified. See the [GPU compatibility audit](docs/GPU_COMPATIBILITY.md) for requirements, evidence and the remaining validation.

## Credits and license

Original plugin code, testing tools and integration extension in this repository are MIT licensed. CoE5 and CoE5SE belong to their respective authors and are separate dependencies; the repository's license does not apply to their files. The investigation used the installed game, native runtime probes, REA/Ghidra and AMD PresentMon. No game executable, decompiled game source, extracted game art, user saves or upstream CoE5SE source is included.

Thanks to **musical_pianist** for the texture-array-batching contribution that informed the multi-page batching and state-tracking work in 0.1.8. These ideas were adapted to this plugin with additional graphics-state restoration and local GPU/image tests.
