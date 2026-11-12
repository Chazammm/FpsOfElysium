# Optional precise frame-rate limiter

The native 5.43 frame loop compares elapsed GetTickCount milliseconds with an integer interval and calls Sleep for the apparent remainder. It accounts for elapsed work; it does not intentionally add the full interval to every frame. However, the low-resolution clock can report no elapsed tick even after several milliseconds of work. In the instrumented large-map scene, the native 3-ms limiter still requested waits in roughly 135 of 300 frames, averaging about 1.5–1.7 ms of actual native sleep per frame despite a mean loop interval above 11 ms.

[Microsoft documents GetTickCount's typical 10–16 ms clock resolution](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-gettickcount). This explains a plausible mechanism for the measured unnecessary waits; a 240 Hz display does not by itself establish which other render, driver or scheduling costs remain.

## Implementation and controls

Graphics 2 → **Precise frame rate limit** is off by default and uses the target selected in Graphics 1. A 240 target means a 4.1667-ms pacing budget. A 360 target means 360, rather than depending on the old native approximation to reach a lower actual rate. Choose the intended target directly.

The enabled mode uses QPC and a high-resolution, one-shot Windows waitable timer. Deadlines advance with the selected cadence; missed deadlines are skipped instead of accumulating a catch-up backlog. Long pauses, focus/target changes and frame-path changes reset the schedule. Only the two known frame-limit Sleep calls are suppressed, after the new pacer has taken responsibility for that frame. GetTickCount always returns its original real result. Other sleeps and all rendering retain their original behavior. There is no spin loop, overclock, voltage change or driver-power-policy change. Rendering in the background is capped at 30 FPS.

The two timing imports and four callsite instructions/targets are checked before hook installation. A conflict or unsupported timer leaves native waiting active. Disable/re-enable also refuses a subsequently redirected timing import. After disabling, installed wrappers simply forward the original calls. The timer is bounded to one process-lifetime handle. Runtime wait failures disable pacing and restore native sleeping. Unexpected third-party timing mutation while enabled is not continuously detected.

[QPC provides high-resolution interval timestamps](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter). The [high-resolution waitable-timer flag requires Windows 10 version 1803 or later](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw). Timer waits have finite timeouts and bounded retries; scheduler and rendering delays can still make individual frames late.

## Laboratory observations, 2026-11-12

Windows, AMD RX 7900 XTX, exact CoE5 5.43, isolated native harness with the 0.1.8 renderer headers, Ultra, camera (40,40), plane 0, requested 1280 × 720, 16 process-local workers. Separate 20-second launches used the same synthetic map. The native 3-ms setting corresponds to the existing approximate 360 menu option; the precise prototype used an explicit 240 target. The first 300-frame block was warmup. These are QPC samples at a once-per-loop boundary, not foreground PresentMon presentation measurements. Animated scene details vary slightly between launches.

| Scene | Native 3-ms limiter | Precise prototype |
| --- | --- | --- |
| Large map, zoom 12 | Steady mean loop intervals around 11.3–11.6 ms; native sleeps around 1.5–1.7 ms/frame | Around 9.6–9.8 ms; no native limiter sleep; the scene itself remains slower than 4.167 ms |
| Light map, zoom 64 | Around 4.5 ms with comparatively uneven native waits | Refined deadline schedule: overall steady mean about 4.172 ms (239.7 loops/s) at the 240 target, without a spin loop; individual stable blocks near 4.167 ms |

An initial relative-deadline prototype averaged around 4.45 ms in the light scene. Advancing the deadline on the intended cadence removed that systematic drift. A yield-only experiment did not improve the large-scene result and was not adopted.

The large-scene prototype increased process CPU time from roughly 3 ms/frame to roughly 9 ms/frame in the recorded runs. This is a real efficiency tradeoff in these samples, not a claim that the timer alone consumes 6 ms or proof of which internal work became more CPU-intensive. Light-scene CPU samples were much smaller and quantized. The option is therefore opt-in; it does not promise a free performance increase or 240 FPS on the large map.

With the refined prototype enabled, image/state checks at zoom 64 found 3, 1 and 2 differing RGBA channels out of 3,686,400, each at most 1/255. Background restoration was exact, checked final state matched and neither path reported GL errors. Cache-refresh, layer-reuse, deletion, VAO and multi-page/sampler GPU fixtures passed. Rendering headers are unchanged in 0.1.9. Actual CoE5SE startup tests verified disabled mode leaves timing imports native, enabled mode owns both imports, background pacing activates, native interval/quality remain unchanged and explicit --maxfps=60 is retained. Unit tests cover the timing guard, real-clock/other-sleep forwarding and live-toggle persistence.

The actual Graphics 2 checkbox was also tested in the isolated main menu: enabling it changed the added Graphics 1 labels from approximate to numeric targets; selecting 120 then 240 changed the active pacing target accordingly. Disabling it restored native waiting and the approximate labels. Each change was persisted successfully, and Ultra remained selected.

Version 0.1.10 subsequently standardizes these option names: both timing modes now show the same numeric "Frames per second" labels, without "(approx.)". The tooltip retains the native interval explanation when precise pacing is off. This label change does not change either limiter's timing behavior.

## Hardware load and power

Higher throughput can increase CPU/GPU power, heat and fan activity. [AMD explains that bounding FPS can reduce those costs](https://www.amd.com/en/products/software/adrenalin/frtc.html); its FRTC product support list is not a claim that that separate driver feature supports this OpenGL game. The custom limiter controls game pacing directly and retains a finite target.

The implementation makes no changes to hardware clocks, voltage, power limits, fan policy or thermal protection. This is ordinary software load, not hardware tuning. That does not certify a particular system's cooling or stability. Actual GPU board watts, hotspot temperature, fan speed and thermal throttling were **not measured** during this investigation; no hardware-temperature or lifetime guarantee follows from the frame-time tests. AMD documents hotspot monitoring for the RX 7900 XTX class in its [performance-tuning guide](https://www.amd.com/en/resources/support-articles/faqs/DH3-020.html).

No new timeBeginPeriod request is made. [Microsoft notes that tighter periodic timer resolution can interfere with power-saving states](https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod); [one-shot waitable timers avoid a continuously periodic timer](https://learn.microsoft.com/en-us/windows/win32/sync/waitable-timer-objects). This is a design choice, not a measured energy saving. The practical efficiency check still needs comparable foreground runs with board-power and temperature readings. NVIDIA execution and longer sessions remain unverified.
