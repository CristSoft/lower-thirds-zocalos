param([string]$Installer=(Join-Path $PSScriptRoot '../install-windows.ps1'))
$ErrorActionPreference='Stop'
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('jcp-installer-'+[guid]::NewGuid().ToString('N'))
$package=Join-Path $fixture 'paquete [prueba] con espacios'
$payload=Join-Path $package 'jcp-sur-lower-thirds'
$portable=Join-Path $fixture 'OBS portable'
New-Item -ItemType Directory -Path (Join-Path $payload 'bin/64bit'),(Join-Path $payload 'data/locale'),(Join-Path $payload 'data/fonts'),(Join-Path $portable 'bin/64bit') -Force | Out-Null
Copy-Item -LiteralPath $Installer -Destination (Join-Path $package 'install-windows.ps1')
$scriptPath=Join-Path $package 'install-windows.ps1'
[IO.File]::WriteAllText((Join-Path $payload 'bin/64bit/jcp-sur-lower-thirds.dll'),'fixture-v1')
[IO.File]::WriteAllText((Join-Path $payload 'data/locale/es-ES.ini'),'Source.Name=fixture')
[IO.File]::WriteAllText((Join-Path $portable 'bin/64bit/obs64.exe'),'fixture-only')
[IO.File]::WriteAllText((Join-Path $payload 'data/fonts/AdventSans-Logo.otf'),'fixture-font')
[IO.File]::WriteAllText((Join-Path $payload 'data/fonts/OFL.txt'),'fixture-license')
# Only this isolated test scope replaces process detection; all writes below
# target the temporary mock OBS folder, never the user's running installation.
function Get-Process { [CmdletBinding()]param([string]$Name) return @() }
function Assert($condition,[string]$message){if(!$condition){throw $message}}
try{
    $plan=& $scriptPath -ObsRoot $portable -ValidateOnly
    Assert ($plan.SourceDLL -eq (Join-Path $payload 'bin/64bit/jcp-sur-lower-thirds.dll')) 'Cannot locate DLL beside packaged installer'
    & $scriptPath -ObsRoot $portable
    $installed=Join-Path $portable 'obs-plugins/64bit/jcp-sur-lower-thirds.dll'
    Assert ([IO.File]::ReadAllText($installed) -eq 'fixture-v1') 'DLL not copied'
    Assert (Test-Path -LiteralPath (Join-Path $portable 'data/obs-plugins/jcp-sur-lower-thirds/locale/es-ES.ini')) 'Resources not copied'
    Assert ([IO.File]::ReadAllText((Join-Path $portable 'data/obs-plugins/jcp-sur-lower-thirds/fonts/AdventSans-Logo.otf')) -eq 'fixture-font') 'Bundled font not copied'
    Assert (Test-Path -LiteralPath (Join-Path $portable 'data/obs-plugins/jcp-sur-lower-thirds/fonts/OFL.txt')) 'Font license not copied'
    [IO.File]::WriteAllText((Join-Path $payload 'bin/64bit/jcp-sur-lower-thirds.dll'),'fixture-v2')
    & $scriptPath -ObsRoot $portable
    Assert ([IO.File]::ReadAllText($installed) -eq 'fixture-v2') 'Update did not replace DLL'
    $missing=Join-Path $fixture 'incomplete'
    New-Item -ItemType Directory -Path $missing -Force | Out-Null
    Copy-Item -LiteralPath $Installer -Destination (Join-Path $missing 'install-windows.ps1')
    $rejected=$false
    try{& (Join-Path $missing 'install-windows.ps1') -ValidateOnly}catch{$rejected=$_.Exception.Message.Contains('Extra')}
    Assert $rejected 'Incomplete ZIP must explain that the full package is required'
    $rootPlan=& $Installer -ValidateOnly
    Assert (Test-Path -LiteralPath $rootPlan.SourceDLL) 'Source checkout fallback must find built DLL'
    $bytes=[IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $Installer))
    Assert ($bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) 'Windows PowerShell 5.1 needs UTF-8 BOM for Spanish text'
    Write-Output 'PASS: packaged layout, source checkout fallback, portable installation, updates, spaces/brackets, bundled font/license, missing payload and UTF-8 BOM'
}finally{
    # Delete only the uniquely created temporary fixture after checking its path.
    $resolved=[IO.Path]::GetFullPath($fixture)
    $tempRoot=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')+'\'
    if(!$resolved.StartsWith($tempRoot,[StringComparison]::OrdinalIgnoreCase) -or !(Split-Path -Leaf $resolved).StartsWith('jcp-installer-')){throw 'Unsafe test cleanup path'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
