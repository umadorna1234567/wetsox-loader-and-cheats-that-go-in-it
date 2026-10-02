param(
    [ValidateSet('farcry4','farcry5')][string]$Game = 'farcry5',
    [string]$EnginePath,
    [ValidateRange(1,60)][int]$Samples = 15
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$helper = Join-Path $PSScriptRoot 'WetsoxGameLoader.exe'
$processName = if ($Game -eq 'farcry4') { 'FarCry4' } else { 'FarCry5' }
$engineName = if ($Game -eq 'farcry4') { 'FC64.dll' } else { 'FC_m64.dll' }
$packName = if ($Game -eq 'farcry4') { 'WetsoxFC4.dll' } else { 'WetsoxFC5.dll' }
$lines = [Collections.Generic.List[string]]::new()
$lines.Add("Wetsox diagnostic report | $Game | $(Get-Date -Format o)")
$lines.Add('This report reads file metadata and existing runtime status; it does not load a cheat or change settings.')
$processes = @(Get-Process -Name $processName -ErrorAction SilentlyContinue)
$target = $null
$loadedPack = $null
if ($processes.Count -eq 1) {
    $target = $processes[0]
    try {
        $modules = @($target.Modules)
        $engine = $modules | Where-Object ModuleName -eq $engineName | Select-Object -First 1
        $loadedPack = $modules | Where-Object ModuleName -eq $packName | Select-Object -First 1
        if (!$EnginePath -and $engine) { $EnginePath = $engine.FileName }
        $lines.Add("Executable version: $($target.MainModule.FileVersionInfo.FileVersion)")
    } catch { $lines.Add('Could not inspect process modules. Run at the same permission level as the game.') }
} else { $lines.Add("Running game process count: $($processes.Count); runtime capture requires exactly one.") }
if ($EnginePath) {
    try {
        $file = Get-Item -LiteralPath $EnginePath
        $lines.Add("Engine: $($file.Name) | File version: $($file.VersionInfo.FileVersion) | Product version: $($file.VersionInfo.ProductVersion) | Bytes: $($file.Length)")
        $lines.Add("Engine SHA-256: $((Get-FileHash -LiteralPath $EnginePath -Algorithm SHA256).Hash)")
        # Section fingerprints allow comparison of code/data versus resource-only changes.
        # Read bytes only: never load an unknown engine DLL into any process.
        $bytes = [IO.File]::ReadAllBytes($file.FullName)
        if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes,0) -ne 0x5A4D) { throw 'Invalid DOS header' }
        $pe = [BitConverter]::ToInt32($bytes,60)
        if ($pe -lt 64 -or $pe + 24 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550) { throw 'Invalid PE header' }
        $count = [BitConverter]::ToUInt16($bytes,$pe+6)
        $optional = [BitConverter]::ToUInt16($bytes,$pe+20)
        $table = $pe + 24 + $optional
        if ($count -gt 96 -or $table + 40*$count -gt $bytes.Length) { throw 'Invalid section table' }
        $lines.Add("PE timestamp: $([BitConverter]::ToUInt32($bytes,$pe+8)) | Machine: $([BitConverter]::ToUInt16($bytes,$pe+4))")
        $hash = [Security.Cryptography.SHA256]::Create()
        try {
            for ($i=0; $i -lt $count; $i++) {
                $entry = $table + 40*$i
                $name = [Text.Encoding]::ASCII.GetString($bytes,$entry,8).TrimEnd([char]0)
                $rva = [BitConverter]::ToUInt32($bytes,$entry+12)
                $size = [BitConverter]::ToUInt32($bytes,$entry+16)
                $offset = [BitConverter]::ToUInt32($bytes,$entry+20)
                if ([long]$offset+$size -gt $bytes.Length) { throw 'Section outside file' }
                $digest = [BitConverter]::ToString($hash.ComputeHash($bytes,[int]$offset,[int]$size)).Replace('-','')
                $lines.Add("Section $name | RVA $rva | Raw bytes $size | SHA-256 $digest")
            }
        } finally { $hash.Dispose() }
        if (Test-Path -LiteralPath $helper) {
            $ErrorActionPreference = 'Continue'
            $check = & $helper --game $Game --check-build $file.FullName 2>&1
            $ErrorActionPreference = 'Stop'
            foreach ($line in $check) { $lines.Add([string]$line) }
        }
    } catch { $lines.Add("Engine inspection failed: $($_.Exception.Message)") }
} else { $lines.Add('Engine not found. Load a save first, or supply -EnginePath with the engine DLL location.') }
if ($loadedPack -and (Test-Path -LiteralPath $helper)) {
    $lines.Add("Loaded pack SHA-256: $((Get-FileHash -LiteralPath $loadedPack.FileName -Algorithm SHA256).Hash)")
    Write-Host "Return to the game and hold your aim binding on a nearby target for $Samples seconds."
    for ($sample=0; $sample -lt $Samples; $sample++) {
        $ErrorActionPreference = 'Continue'
        $status = & $helper --game $Game --pid $target.Id --runtime-status $loadedPack.FileName 2>&1
        $ErrorActionPreference = 'Stop'
        $lines.Add("Sample $sample | $(Get-Date -Format HH:mm:ss.fff)")
        foreach ($line in $status) { $lines.Add([string]$line) }
        Start-Sleep -Milliseconds 1000
    }
} else { $lines.Add('No loaded Wetsox game pack found; runtime capture skipped.') }
$directory = Join-Path $root 'diagnostics'
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$report = Join-Path $directory ("$Game-" + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.txt')
$lines | Set-Content -LiteralPath $report -Encoding UTF8
Write-Host "Report saved: $report"
