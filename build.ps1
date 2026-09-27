param(
    [string]$QtPath = "$PSScriptRoot\tools\qt\6.8.3\msvc2022_64",
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$Run
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath "$QtPath\lib\cmake\Qt6\Qt6Config.cmake")) {
    throw "Qt SDK not found at $QtPath. Pass -QtPath pointing to an installed MSVC Qt 6.5+ kit."
}
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstall) { throw 'Install Visual Studio with the Desktop development with C++ workload.' }
$vcvars = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars64.bat'
# Import the compiler environment into this process only.
$compilerEnvironment = & $env:ComSpec /d /c "call `"$vcvars`" >nul && set"
$compilerEnvironment | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$' -and $matches[1] -ine 'PATH') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}
$compilerPath = ($compilerEnvironment | Where-Object { $_ -cmatch '^PATH=' } | Select-Object -First 1) -replace '^PATH=', ''
if (-not $compilerPath) { $compilerPath = ($compilerEnvironment | Where-Object { $_ -imatch '^Path=' } | Select-Object -First 1) -replace '^Path=', '' }
[Environment]::SetEnvironmentVariable('PATH', $null, 'Process')
[Environment]::SetEnvironmentVariable('Path', $null, 'Process')
$env:PATH = $compilerPath
if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the MSVC compiler environment.' }
$ninja = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$buildDir = Join-Path $PSScriptRoot "build-$Configuration"
& cmake -S $PSScriptRoot -B $buildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_PREFIX_PATH=$QtPath" "-DCMAKE_BUILD_TYPE=$Configuration"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& cmake --build $buildDir --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
# Keep the development executable launchable directly from Explorer, too.
& "$QtPath\bin\windeployqt.exe" --no-translations --qmldir "$PSScriptRoot\qml" "$buildDir\Wetsox.exe"
if ($LASTEXITCODE -ne 0) { throw 'Development runtime deployment failed.' }
$env:PATH = "$QtPath\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$QtPath\plugins"
$env:QML_IMPORT_PATH = "$QtPath\qml"
& ctest --test-dir $buildDir --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
& cmake --install $buildDir --prefix "$PSScriptRoot\out"
if ($LASTEXITCODE -ne 0) { throw 'Deployment failed.' }
# Remove only known obsolete launcher/module copies after successful deployment.
$deployedBin = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'out\bin'))
$currentModule = Join-Path $deployedBin 'cheats\farcry5\WetsoxFC5.dll'
if (-not (Test-Path -LiteralPath $currentModule)) { throw 'The deployed game module is missing.' }
foreach ($legacyName in @('NexusFC5.dll','NexusFC5_v2.dll','NexusFC5_v3.dll','NexusFC5_v4.dll','NexusFC5_v5.dll','Nexus-updated.exe','Nexus.exe','NexusGameLoader.exe')) {
    $legacyPath = [IO.Path]::GetFullPath((Join-Path $deployedBin $legacyName))
    if ([IO.Path]::GetDirectoryName($legacyPath) -ne $deployedBin) { throw 'Invalid legacy cleanup path.' }
    if (Test-Path -LiteralPath $legacyPath) {
        try { Remove-Item -LiteralPath $legacyPath -ErrorAction Stop }
        catch { Write-Warning "Could not remove $legacyName. Close the application using it and rebuild." }
    }
}
$oldNestedModule = [IO.Path]::GetFullPath((Join-Path $deployedBin 'cheats\farcry5\NexusFC5.dll'))
if (-not $oldNestedModule.StartsWith($deployedBin + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid old module path.' }
if (Test-Path -LiteralPath $oldNestedModule) {
    try { Remove-Item -LiteralPath $oldNestedModule -ErrorAction Stop }
    catch { Write-Warning 'The old NexusFC5.dll is still in use. Close Far Cry 5 and rebuild to remove it.' }
}
if ($Run) { & "$PSScriptRoot\out\bin\Wetsox.exe" }
