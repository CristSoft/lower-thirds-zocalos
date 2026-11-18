param([ValidateSet('Release','RelWithDebInfo')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
Push-Location -LiteralPath $PSScriptRoot
try {
    if (Get-Command python -ErrorAction SilentlyContinue) {
        python tools/fetch-dependencies.py
        if ($LASTEXITCODE -ne 0) { throw 'No se pudieron descargar las dependencias. Revisá la conexión y volvé a ejecutar.' }
    }
    cmake --preset windows-x64 -DJCP_BUILD_TESTS=ON
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo configurar la compilación.' }
    cmake --build build_x64 --config $Configuration --parallel
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo compilar el plugin.' }
    # Qt is supplied by OBS at runtime; use that same runtime for the checks.
    $qtPath=Join-Path $PSScriptRoot '.deps/obs-deps-qt6-2025-07-11-x64'
    $env:PATH=(Join-Path $qtPath 'bin')+';'+$env:PATH
    $env:QT_PLUGIN_PATH=Join-Path $qtPath 'plugins'
    ctest --test-dir build_x64 -C $Configuration --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Fallaron las verificaciones.' }
    powershell.exe -NoProfile -File tests/check-installer.ps1
    if ($LASTEXITCODE -ne 0) { throw 'Fallaron las verificaciones del instalador.' }
    cmake --install build_x64 --config $Configuration --prefix release
    if ($LASTEXITCODE -ne 0) { throw 'No se pudo preparar el paquete.' }
    Copy-Item -LiteralPath 'install-windows.ps1','README.md','LICENSE' -Destination release
    Compress-Archive -Path 'release/*' -DestinationPath '../Plugin-OBS-JCP-Sur-Windows-x64.zip' -Force
    $packageVersion=(Get-Content -LiteralPath 'buildspec.json' -Raw | ConvertFrom-Json).version
    Copy-Item -LiteralPath '../Plugin-OBS-JCP-Sur-Windows-x64.zip' -Destination (Join-Path '..' ('Plugin-OBS-JCP-Sur-v'+$packageVersion+'-Windows-x64.zip')) -Force
} finally { Pop-Location }
