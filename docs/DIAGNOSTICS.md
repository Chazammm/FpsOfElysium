# Local startup diagnostics

Log location: `coe5mod.log` next to the executable that was launched. For a normal Steam installation, this is the game's installation directory. Search for `fps_mod` to isolate this plugin's messages.

| Message | Meaning |
| --- | --- |
| `startup version=0.1.10-local` | Identifies this local build before its checks run. |
| `hook_check status=passed imports=48 callsites=8` | The import targets and pinned game calls matched before graphics imports were changed, including both map-quality paths. |
| `map_renderer quality=... path=... batching_requested=1` | The selected quality reached its optimized map scope. Values 9/10/11 are Low/Medium/High and use `low_medium_high`; 12/13 are Highest/Ultra and use `highest_ultra`. Emitted once for each quality used. Unsupported individual textures/states can still use original drawing. |
| `frame_limit interval_ms=... unlimited=... source=...` | Native FPS interval. `preference_pending` is applied once after configuration loading; `preference` confirms that restore. `launch_option` preserves an explicit `--maxfps` argument. `graphics_menu saved=1` confirms a selection was persisted. In 0.1.7, `legacy_unlimited_migrated` reports conversion of an old zero-delay setting to the bounded 3-ms target. |
| `gpu_wait glfinish=0 source=...` | Extra GPU synchronization is off. Source is the game state, saved mod preference, explicit launch option or a live graphics-menu change. `saved=1` means that menu change was written successfully. |
| `precise_frame_limit enabled=... source=... target=...` | The optional high-resolution limiter is on/off. Its default is off. The target is the chosen frame-rate value or explicit startup target. `busy_wait=0 game_clock_unchanged=1` describes its blocking timer and real game-clock forwarding. |
| `precise_frame_limit active_target=30 foreground=0` | Background throttling is active. Returning the game to the foreground restores its selected target. This line appears on target changes, not every frame. |
| `precise_frame_limit enabled=0 reason=... native_wait_restored=1` | Timing-hook conflict, missing timer capability or a wait failure disabled the option. Rendering remains available; the pacer does not overwrite a foreign timing import. |
| `graphics_menu ready page=Graphics_2` | The native Graphics 2 checkbox is available. It controls the existing glFinish flag, not VSync or the FPS limit. |
| `graphics vendor=... renderer=... opengl_driver=...` | Actual current OpenGL device/driver, logged once at the main menu or first map. The driver text is the OpenGL implementation string, not a separate Windows package-version lookup. |
| `initialized` | Plugin initialization completed. Graphics resources may still await the first map draw. |
| `renderer_ready` | The texture-array renderer initialized and uploaded supported textures. `multi_page=1 state_tracking=1` identifies the two 0.1.8 additions. `multi_page=0` can mean the required texture-unit or sampler-binding capability is unavailable; the previous single-page path remains usable. |
| `original_path reason=...` | A draw used original rendering for the named unsupported condition. This does not mean every draw or the entire mod is disabled. Each cause appears at most once per launch. |
| `disabled reason=graphics_import_conflict function=... actual=... expected=...` | A required GL/GLU import was already redirected. This plugin leaves graphics imports unchanged and does not install its renderer hooks. |
| `disabled reason=game_callsite_changed` | A pinned game call differs in memory. The following startup check message states whether graphics imports were untouched. |
| `disabled reason=unsupported_game_hash` | The executable differs from the supported CoE5 build; no graphics patches are applied. |

Normal map assets can produce `texture_size_unsupported`, `texture_format_unsupported` or `texture_mip_chain_unsupported` for a subset of textures. Those objects retain original drawing. Other categories identify bound buffers/VAOs, custom shaders, unsupported fixed-function state, texture-cache limits, missing GL capabilities, or shader compilation/link errors. Compiler/linker details are included when available.

The guard compares the executable's import addresses to the loaded modules' export addresses, including the game's GLU forwarder. It does not inspect arbitrary instructions inside exported functions, recognize every third-party proxy, or continuously watch for hooks installed after startup. It is a conservative conflict check for the specific import slots and game calls this plugin uses.

No dialogs, graphics settings or quality reductions are added by diagnostics. Reporting is bounded; normal frames do not write repeated diagnostic lines or collect frame timings.
