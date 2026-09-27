$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$bin = Join-Path $PSScriptRoot 'out\bin'
$destination = Join-Path $PSScriptRoot 'nexus stuff'
$games = @(@{ Id='farcry5'; Title='Far Cry 5'; Module='WetsoxFC5.dll'; Archive='Wetsox-FarCry5.zip' }, @{ Id='farcry4'; Title='Far Cry 4'; Module='WetsoxFC4.dll'; Archive='Wetsox-FarCry4.zip' })
foreach ($required in @('Wetsox.exe', 'WetsoxGameLoader.exe', 'Qt6Core.dll', 'platforms\qwindows.dll', 'cheats\farcry5\WetsoxFC5.dll', 'cheats\farcry5\game.json', 'cheats\farcry5\cover.jpg', 'cheats\farcry4\WetsoxFC4.dll', 'cheats\farcry4\game.json', 'cheats\farcry4\cover.jpg')) {
    if (-not (Test-Path -LiteralPath (Join-Path $bin $required) -PathType Leaf)) { throw "Missing deployed file: $required. Run build.ps1 first." }
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
        if ($emptyCheats) { $archive.CreateEntry('cheats/') | Out-Null }
    } finally { $archive.Dispose() }
}

# Explicit runtime selection excludes settings, configs, game packs and loose logs.
$loaderFiles = @(Get-ChildItem -LiteralPath $bin -File | Where-Object {
    $_.Extension -eq '.dll' -or $_.Name -in @('Wetsox.exe', 'WetsoxGameLoader.exe', 'vc_redist.x64.exe', 'qt.conf')
})
foreach ($folder in @('generic','iconengines','imageformats','licenses','networkinformation','platforms','qml','qmltooling','tls')) {
    $path = Join-Path $bin $folder
    if (Test-Path -LiteralPath $path) { $loaderFiles += @(Get-ChildItem -LiteralPath $path -Recurse -File) }
}
$loaderInstructions = @'
Wetsox loader - Windows x64

Extract this entire archive into a writable folder. Keep all DLLs and folders.
Run Wetsox.exe. If Windows reports a missing Visual C++ runtime, run the included vc_redist.x64.exe.
Install game packs inside the cheats folder beside Wetsox.exe.
For Far Cry 5, use cheats/farcry5/game.json (plus its DLL and cover).
For Far Cry 4, use cheats/farcry4/game.json (plus its DLL and cover).
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
The finished layout is Wetsox.exe beside cheats/$($game.Id)/$($game.Module), game.json and cover.jpg.
Keep the licenses folder with the pack. The card appears automatically in Wetsox.
Start $($game.Title), load a single-player save, then click its card in Wetsox.
"@
    # Explicit selection prevents development probes, settings and configs leaking into packs.
    $gameFiles = @($game.Module, 'game.json', 'cover.jpg', 'licenses\imgui.txt', 'licenses\minhook.txt') |
        ForEach-Object { Get-Item -LiteralPath (Join-Path $pack $_) }
    $temporary = Join-Path $destination ([guid]::NewGuid().ToString() + '.zip')
    Write-Package $temporary $gameFiles $pack ($game.Id + '/') $gameInstructions $false
    Move-Item -LiteralPath $temporary -Destination (Join-Path $destination $game.Archive) -Force
}
Set-Content -LiteralPath (Join-Path $destination 'README.txt') -Encoding UTF8 -Value @'
Upload these as three separate downloads:

Wetsox-loader.zip - the base application and its required runtime files.
Wetsox-FarCry5.zip - the Far Cry 5 module, cover, manifest and license notices.
Wetsox-FarCry4.zip - the Far Cry 4 module, cover, manifest and license notices.

Users extract the loader, then copy farcry4 or farcry5 from the corresponding game archive into its cheats folder.
None of the downloads contains your personal appearance settings or saved configs.
Rebuild these downloads with: powershell -NoProfile -ExecutionPolicy Bypass -File .\package.ps1
'@
Get-ChildItem -LiteralPath $destination -File | Select-Object Name, Length
