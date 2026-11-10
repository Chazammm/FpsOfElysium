param([ValidatePattern('^\d+\.\d+\.\d+$')][string]$Version='0.1.3')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'build'
$plugin=Join-Path $out 'FPSofElysiumSE.c5plugin.dll'
$hash=(Get-FileHash -LiteralPath $plugin -Algorithm SHA256).Hash
$name="FPSofElysiumSE-$Version-experimental"
$stage=Join-Path $out $name;$zip="$stage.zip"
if((Test-Path -LiteralPath $stage) -or (Test-Path -LiteralPath $zip)) { throw 'Release destination already exists.' }
New-Item -ItemType Directory -Path (Join-Path $stage 'mods') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'mod\FPSofElysiumSE') -Destination (Join-Path $stage 'mods') -Recurse
Copy-Item -LiteralPath $plugin -Destination (Join-Path $stage 'mods\FPSofElysiumSE')
foreach($file in @('README.md','LICENSE','CHANGELOG.md')) { Copy-Item -LiteralPath (Join-Path $root $file) -Destination $stage }
[ordered]@{version="$Version-experimental";plugin_sha256=$hash;game='CoE5 5.43 Windows x64';game_sha256='489d0b82fa790ec36e69a23e2335d0c2048fa57f4051fe8159c64f2d33508e55';extender='CoE5SE 3.2.2 API 11';adapted_loader_included=$false;loader_requirement='Build the matching exact-hash adapted loader from separately obtained official CoE5SE source using scripts/Build.ps1.'} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $stage 'manifest.json') -Encoding UTF8
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -CompressionLevel Optimal
$zipHash=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
"$zipHash  $name.zip" | Set-Content -LiteralPath (Join-Path $out "$name.SHA256.txt") -Encoding ASCII
[pscustomobject]@{Archive=$zip;SHA256=$zipHash;PluginSHA256=$hash;AdaptedLoaderIncluded=$false}
