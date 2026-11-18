param([string]$Iscc = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (!$Iscc) {
    $uninstallKeys = @('HKCU:/Software/Microsoft/Windows/CurrentVersion/Uninstall', 'HKLM:/Software/Microsoft/Windows/CurrentVersion/Uninstall', 'HKLM:/Software/WOW6432Node/Microsoft/Windows/CurrentVersion/Uninstall')
    $installed = Get-ChildItem $uninstallKeys -ErrorAction SilentlyContinue | Get-ItemProperty -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match '^Inno Setup' -and $_.InstallLocation } | ForEach-Object { Join-Path $_.InstallLocation 'ISCC.exe' }
    $candidates = @($installed) + @(
        (Join-Path $root '.deps/inno/Inno Setup 6/ISCC.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6/ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 7/ISCC.exe')
    )
    $Iscc = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
}
if (!$Iscc) { throw 'No se encontró el compilador ISCC de Inno Setup. Instalá Inno Setup o indicá su ruta con -Iscc.' }
if (!(Test-Path -LiteralPath (Join-Path $root 'release/jcp-sur-lower-thirds/bin/64bit/jcp-sur-lower-thirds.dll') -PathType Leaf)) {
    throw 'Falta la compilación Release. Compilá e instalá primero el contenido en la carpeta release.'
}
& $Iscc (Join-Path $PSScriptRoot 'zocalos.iss')
if ($LASTEXITCODE -ne 0) { throw 'No se pudo generar el instalador.' }
