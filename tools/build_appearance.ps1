$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$qtKit = Join-Path $taskRoot 'tools\qt\6.8.3\msvc2022_64'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsInstall = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsInstall 'VC\Auxiliary\Build\vcvars64.bat'
$compilerEnvironment = & $env:ComSpec /d /c "call `"$vcvars`" >nul && set"
foreach ($line in $compilerEnvironment) { if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') } }
$ninja = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
$buildDir = Join-Path $taskRoot 'build-appearance'
& cmake -S $taskRoot -B $buildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" "-DCMAKE_PREFIX_PATH=$qtKit" '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& cmake --build $buildDir --parallel 6
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
$deploymentPreference = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
& "$qtKit\bin\windeployqt.exe" --no-translations --qmldir "$taskRoot\qml" "$buildDir\backend\WetsoxApp.exe"
$deploymentExit = $LASTEXITCODE
$ErrorActionPreference = $deploymentPreference
if ($deploymentExit -ne 0) { throw 'Runtime deployment failed' }
$env:PATH = "$qtKit\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$qtKit\plugins"
$env:QML_IMPORT_PATH = "$qtKit\qml"
& ctest --test-dir $buildDir --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
$ErrorActionPreference = 'Continue'
& cmake --install $buildDir --prefix "$taskRoot\out"
$deploymentExit = $LASTEXITCODE
$ErrorActionPreference = $deploymentPreference
if ($deploymentExit -ne 0) { throw 'Install failed' }

exit 0

