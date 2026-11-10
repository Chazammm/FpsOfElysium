# FpsOfElysium

Experimental rendering optimization for **Conquest of Elysium 5.43, Windows x64**, using **CoE5SE 3.2.2 / API 11**. It reduces the overhead of drawing large, zoomed-out maps while retaining the selected Highest/Ultra quality.

The original map path submits tens of thousands of small decoration draws. This mod batches compatible consecutive decorations using texture arrays, preserves their original texture coordinates and complete existing mip chains, defers redundant source-texture bindings, and reuses client-array setup between compatible batches. It preserves object order and uses the original path for unsupported textures and states. It changes no gameplay, camera controls, worker count or quality setting.

**This is an experimental native plugin with an explicit loader adaptation. Stock CoE5SE 3.2.2 does not load external native plugins.** The adaptation admits only the exact locally built FPSofElysiumSE DLL hash and retains the loader's normal approval process. This is not an official CoE5SE release or a Workshop-only mod.

## Results and quality checks

On an RX 7900 XTX, the copied real-save benchmark at Ultra improved from **36.5 FPS off to 64.0 FPS on (+75.2%)**. That full FPS measurement used version 0.1.0; it has not been repeated for 0.1.2. Both 15-second captures used a requested 2560 × 1440 window and the same read-back plane, zoom and camera. Separate launches mean the animated scenes were not pixel-identical.

Version 0.1.2 additionally reduces repeated texture binding and client-array setup. See [measurement details](docs/PERFORMANCE.md) for section timings and their limits. Image comparisons retain all details and have shown only a handful of RGBA channel differences, at most 1/255. The images are not perfectly bit-identical. These are targeted tests, not a guarantee for every GPU, plane or mod combination.

## Build

Dependencies are obtained separately: the user's own CoE5 5.43 installation, the complete CoE5SE 3.2.2 installation including its source directory, and llvm-mingw for Windows x64. The development compiler was llvm-mingw 20260922 UCRT x86_64.

```powershell
.\scripts\Build.ps1 -Compiler 'C:\tools\llvm-mingw\bin\x86_64-w64-mingw32-clang.exe' -CoE5SESource 'C:\tools\CoE5SE\src'
```

This builds the plugin and a matching adapted loader in `build`, then checks null/API-version and foreign-executable guards. The loader is built from the separately supplied CoE5SE source using its `LOADER_EXTRA_HEADER` extension. Upstream loader/SDK files and compiled loader binaries are not redistributed in this repository. Rebuilding the plugin requires rebuilding its matching loader.

## Test installation and removal

1. Close the game. Use a separate game/profile copy with the full CoE5SE 3.2.2 already installed according to its own instructions. Back up that test profile before changing it.
2. Back up the test profile's `mods/CoE5SE/coe5se_loader.dll`, then replace it with your matching `build/coe5se_loader.dll`. Keep the remaining CoE5SE files from the same 3.2.2 installation.
3. Copy `mod/FPSofElysiumSE` to the same profile's local `mods` directory and add `build/FPSofElysiumSE.c5plugin.dll` inside it. Enable CoE5SE and FPSofElysiumSE and follow the normal plugin approval prompt.
4. The “Smoother map rendering” setting defaults to on. Its startup on/off paths are verified; interactive switching through the Mod Settings screen has not been checked directly.

To remove it, close the game, disable FPSofElysiumSE, and restore the backed-up original loader. Disabling its setting uses the original Highest/Ultra drawing path while the plugin remains loaded.

The exact supported EXE SHA-256 is `489d0b82fa790ec36e69a23e2335d0c2048fa57f4051fe8159c64f2d33508e55`. A different executable receives no code patches. Texture-array caching is limited to 128 MiB. Unsupported contexts use the original path. Long sessions, all planes, multiplayer and arbitrary other native graphics plugins are not comprehensively tested. Normal game data/content mods are not included.

For startup problems inspect `coe5mod.log` beside the launched executable. A successful load reports `fps_mod initialized` and, once the map is drawn, `fps_mod renderer_ready`. A loader and plugin hash mismatch requires rebuilding both as a matching pair. A game hash/signature mismatch leaves the optimization disabled.

## Credits and license

Original plugin code, testing tools and integration extension in this repository are MIT licensed. CoE5 and CoE5SE belong to their respective authors and are separate dependencies; the repository's license does not apply to their files. The investigation used the installed game, native runtime probes, REA/Ghidra and AMD PresentMon. No game executable, decompiled game source, extracted game art, user saves or upstream CoE5SE source is included.
