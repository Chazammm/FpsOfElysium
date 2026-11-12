param(
    [Parameter(Mandatory=$true)][string]$Compiler,
    [Parameter(Mandatory=$true)][string]$CoE5SESource,
    [string]$Windres
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$compilerPath=(Resolve-Path -LiteralPath $Compiler).Path
$sdk=(Resolve-Path -LiteralPath $CoE5SESource).Path
if(!(Test-Path -LiteralPath (Join-Path $sdk 'jazzy_api.h')) -or !(Test-Path -LiteralPath (Join-Path $sdk 'coe5se_loader.c'))) { throw 'CoE5SESource must point to the CoE5SE 3.2.2 src directory.' }
if(!$Windres) { $Windres=Join-Path (Split-Path -Parent $compilerPath) 'x86_64-w64-mingw32-windres.exe' }
$out=Join-Path $root 'build';New-Item -ItemType Directory -Path $out -Force | Out-Null
$plugin=Join-Path $out 'FPSofElysiumSE.c5plugin.dll'
& $compilerPath -std=c11 -O2 -Wall -Wextra -Werror -DFPS_SE_MOD -shared '-Wl,--no-insert-timestamp' -I $sdk (Join-Path $root 'src\fps_mod.c') -lbcrypt -lshell32 -o $plugin
if($LASTEXITCODE -ne 0) { throw 'Plugin build failed.' }
& $compilerPath -std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -I (Join-Path $root 'src') (Join-Path $root 'tests\test_texture_cache.c') -o (Join-Path $out 'test_texture_cache.exe')
if($LASTEXITCODE -ne 0) { throw 'Texture-cache test build failed.' }
& (Join-Path $out 'test_texture_cache.exe')
if($LASTEXITCODE -ne 0) { throw 'Texture-cache regression failed.' }
& $compilerPath -std=c11 -O2 -Wall -Wextra -Werror -I $sdk -I (Join-Path $root 'src') (Join-Path $root 'tests\test_graphics_imports.c') -lbcrypt -lshell32 -o (Join-Path $out 'test_graphics_imports.exe')
if($LASTEXITCODE -ne 0) { throw 'Graphics-import guard test build failed.' }
& (Join-Path $out 'test_graphics_imports.exe')
if($LASTEXITCODE -ne 0) { throw 'Graphics-import conflict or diagnostic regression failed.' }
$hash=(Get-FileHash -LiteralPath $plugin -Algorithm SHA256).Hash
('#define FPS_MOD_EXPECTED_SHA256 "'+$hash+'"') | Set-Content -LiteralPath (Join-Path $out 'fps_mod_hash.h') -Encoding ASCII
$resource=Join-Path $out 'loader-resource.o'
& $Windres -O coff -o $resource (Join-Path $sdk 'coe5se_loader.rc')
if($LASTEXITCODE -ne 0) { throw 'Loader resource build failed.' }
& $compilerPath -shared -O2 -Wall -Wextra -Werror '-Wl,--no-insert-timestamp' -I $out -I $sdk -I (Join-Path $root 'integration') (Join-Path $root 'integration\mod_loader_entry.c') $resource -lbcrypt -luser32 -o (Join-Path $out 'coe5se_loader.dll')
if($LASTEXITCODE -ne 0) { throw 'Adapted loader build failed.' }
& $compilerPath -std=c11 -O2 -Wall -Wextra -Werror -I $sdk (Join-Path $root 'tests\verify_mod_guard.c') -o (Join-Path $out 'verify_mod_guard.exe')
if($LASTEXITCODE -ne 0) { throw 'Guard build failed.' }
Push-Location $out
try { & .\verify_mod_guard.exe;if($LASTEXITCODE -ne 0) { throw 'API/foreign-executable guard test failed.' } } finally { Pop-Location }
Get-FileHash -Algorithm SHA256 -LiteralPath $plugin,(Join-Path $out 'coe5se_loader.dll')
