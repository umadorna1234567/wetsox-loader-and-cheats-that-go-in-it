$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$bin = Join-Path $PSScriptRoot 'out\bin'
$destination = Join-Path $PSScriptRoot 'nexus stuff'
$games = @(@{ Id='farcry5'; Title='Far Cry 5'; Module='WetsoxFC5.dll'; Archive='Wetsox-FarCry5.zip' }, @{ Id='farcry4'; Title='Far Cry 4'; Module='WetsoxFC4.dll'; Archive='Wetsox-FarCry4.zip' }, @{ Id='justcause4'; Title='Just Cause 4'; Module='WetsoxJC4.dll'; Archive='Wetsox-JustCause4.zip' }, @{ Id='killingfloor2'; Title='Killing Floor 2'; Module='WetsoxKF2.dll'; Archive='Wetsox-KillingFloor2.zip' })
foreach ($required in @('Wetsox.exe', 'backend\WetsoxGameLoader.exe', 'backend\Qt6Core.dll', 'backend\platforms\qwindows.dll', 'cheats\farcry5\WetsoxFC5.dll', 'cheats\farcry5\game.json', 'cheats\farcry5\cover.jpg', 'cheats\farcry4\WetsoxFC4.dll', 'cheats\farcry4\game.json', 'cheats\farcry4\cover.jpg')) {
    if (-not (Test-Path -LiteralPath (Join-Path $bin $required) -PathType Leaf)) { throw "Missing deployed file: $required. Run build.ps1 first." }
}
# Validate every game pack before replacing any existing download.
foreach ($game in $games) {
    $pack = Join-Path $bin ('cheats\' + $game.Id)
    $manifest = Get-Content -LiteralPath (Join-Path $pack 'game.json') -Raw | ConvertFrom-Json
    foreach ($required in @($game.Module, 'game.json', $manifest.artwork)) {
        if (-not (Test-Path -LiteralPath (Join-Path $pack $required) -PathType Leaf)) { throw "Missing $($game.Id) pack file: $required" }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $pack 'licenses') -PathType Container)) { throw "Missing $($game.Id) license notices" }
}
New-Item -ItemType Directory -Path $destination -Force | Out-Null

function Add-TextEntry($archive, $name, $content) {
    $entry = $archive.CreateEntry($name)
    $writer = [IO.StreamWriter]::new($entry.Open())
    try { $writer.Write($content) } finally { $writer.Dispose() }
}
function Write-Package($path, $files, $base, $prefix, $instructions, $emptyCheats) {
    $archive = [IO.Compression.ZipFile]::Open($path, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in $files) {
            $relative = $file.FullName.Substring($base.Length + 1).Replace('\', '/')
            [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $file.FullName, $prefix + $relative, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
        }
        Add-TextEntry $archive 'INSTALL.txt' $instructions
        if ($emptyCheats) { $archive.CreateEntry('cheats/') | Out-Null; $archive.CreateEntry('configs/') | Out-Null }
    } finally { $archive.Dispose() }
}

# Explicit runtime selection excludes settings, configs, game packs and loose logs.
$loaderFiles = @((Get-Item -LiteralPath (Join-Path $bin 'Wetsox.exe')))
$loaderFiles += @(Get-ChildItem -LiteralPath (Join-Path $bin 'backend') -Recurse -File | Where-Object { $_.Extension -notin @('.log','.pdb') -and $_.Name -ne 'settings.json' })
$loaderInstructions = @'
Wetsox loader - Windows x64

Extract this entire archive into a writable folder. Keep the backend folder beside Wetsox.exe.
When updating from the old flat layout, use a fresh folder and copy your cheats and configs into it.
Move an old settings.json into configs/settings.json to preserve appearance.
Update the loader and all game packs together; restart the games before using this release. Hotkeys require the matching v8 session protocol.
Run Wetsox.exe. If Windows reports a missing Visual C++ runtime, run backend/vc_redist.x64.exe.
Install game packs inside the cheats folder beside Wetsox.exe.
For Far Cry 5, use cheats/farcry5/game.json (plus its DLL and cover).
For Far Cry 4, use cheats/farcry4/game.json (plus its DLL and cover).
For Just Cause 4, use cheats/justcause4/game.json (plus its DLL and cover).
For Killing Floor 2, use cheats/killingfloor2/game.json (plus its DLL and cover).
Extraction wrapper folders under cheats are supported.
Appearance and configs are created locally when you use the app.
'@
# Build each archive under a unique filename before replacing its previous copy.
$temporary = Join-Path $destination ([guid]::NewGuid().ToString() + '.zip')
Write-Package $temporary $loaderFiles $bin '' $loaderInstructions $true
Move-Item -LiteralPath $temporary -Destination (Join-Path $destination 'Wetsox-loader.zip') -Force
foreach ($game in $games) {
    $pack = Join-Path $bin ('cheats\' + $game.Id)
    $gameInstructions = @"
Wetsox - $($game.Title) pack

Install the latest Wetsox loader separately first.
Close $($game.Title) before replacing an older pack.
Copy the entire $($game.Id) folder from this archive into the loader's cheats folder.
The finished layout is Wetsox.exe beside cheats/$($game.Id)/$($game.Module), game.json and its cover image.
Keep the licenses folder with the pack. The card appears automatically in Wetsox.
Start $($game.Title), load a single-player save or solo match, then click its card in Wetsox.
Use the latest loader and game packs together (session protocol v8). Share diagnostics if features fail.
Controller binds support XInput and native DualSense/DS4. L2/R2 use Pad LT/RT; Cross uses Pad A.
"@
    if ($game.Id -eq 'killingfloor2') {
        $gameInstructions += "`n`nTested with the Steam Windows x64 version in solo play. Gameplay changes run only in a local solo match. No files need to be copied into the game installation. Use Aimbot + Silent aim together for silent targeting; projectile prediction is automatic. Visibility check prevents targeting through walls: disable it when intentionally using aim with Shoot through walls. Noclip controls: WASD horizontal movement, Space up, Ctrl down; use Noclip speed to adjust movement. Rapid fire repeats single-shot primary fire while held; No reload avoids reload pauses. Settings and saved configs stay with the loader under configs."
    }
    if ($game.Id -in @('farcry4','farcry5')) {
        $gameInstructions += "`n`nFar Cry engine hashes are informational: different hashes are allowed to attempt startup. Other builds may still need different gameplay bindings."
    }
    if ($game.Id -eq 'justcause4') {
        $gameInstructions += "`n`nConverted from the supplied Solis gameplay code for Steam build 4110618. Disable the old Solis xinput9_1_0.dll before starting the game; do not run both modules together. Back up that old DLL if you want to restore it. This pack uses the Wetsox menu and does not need a proxy DLL in the game directory. Skystriker and hoverboard features require the corresponding content to be unlocked. Aim assist uses torso estimates and has no wall check. Controller input supports XInput-compatible devices and native DualSense/DS4 controllers."
    }
    # Explicit selection prevents development probes, settings and configs leaking into packs.
    $manifest=Get-Content -LiteralPath (Join-Path $pack 'game.json') -Raw | ConvertFrom-Json
    $gameFiles = @($game.Module, 'game.json', $manifest.artwork) |
        ForEach-Object { Get-Item -LiteralPath (Join-Path $pack $_) }
    $gameFiles += @(Get-ChildItem -LiteralPath (Join-Path $pack 'licenses') -File)
    $temporary = Join-Path $destination ([guid]::NewGuid().ToString() + '.zip')
    Write-Package $temporary $gameFiles $pack ($game.Id + '/') $gameInstructions $false
    Move-Item -LiteralPath $temporary -Destination (Join-Path $destination $game.Archive) -Force
}
Set-Content -LiteralPath (Join-Path $destination 'README.txt') -Encoding UTF8 -Value @'
Upload these as five separate downloads:

Wetsox-loader.zip - the base application and its required runtime files.
Wetsox-FarCry5.zip - the Far Cry 5 module, cover, manifest and license notices.
Wetsox-FarCry4.zip - the Far Cry 4 module, cover, manifest and license notices.
Wetsox-JustCause4.zip - the Just Cause 4 module, cover, manifest and license notices.
Wetsox-KillingFloor2.zip - the Killing Floor 2 module, cover, manifest and license notices.

Users extract the loader, then copy killingfloor2, justcause4, farcry4 or farcry5 from the corresponding game archive into its cheats folder.
None of the downloads contains your personal appearance settings or saved configs.
Rebuild these downloads with: powershell -NoProfile -ExecutionPolicy Bypass -File .\package.ps1
'@
Get-ChildItem -LiteralPath $destination -File | Select-Object Name, Length
