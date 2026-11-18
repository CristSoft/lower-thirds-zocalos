# Installer for the compiled DLL. No script runs inside OBS.
# UTF-8 with BOM preserves Spanish messages in Windows PowerShell 5.1.
param([string]$ObsRoot='', [string]$PluginRoot='', [switch]$ValidateOnly)
$ErrorActionPreference='Stop'
$dllName='jcp-sur-lower-thirds.dll'
$binary=''
$resourceRoot=''
$candidates=if($PluginRoot){@($PluginRoot)}else{@(
    (Join-Path $PSScriptRoot 'jcp-sur-lower-thirds'),
    $PSScriptRoot,
    (Join-Path $PSScriptRoot 'release/jcp-sur-lower-thirds')
)}
foreach($candidate in $candidates){
    $candidateDll=Join-Path $candidate ('bin/64bit/'+$dllName)
    if(Test-Path -LiteralPath $candidateDll -PathType Leaf){
        $binary=(Resolve-Path -LiteralPath $candidateDll).Path
        $resourceRoot=Join-Path $candidate 'data'
        break
    }
}
# Running from the source checkout also works if the release payload was moved
# during manual installation, provided a locally compiled Release DLL exists.
if(!$binary -and !$PluginRoot){
    foreach($repo in @($PSScriptRoot,(Split-Path -Parent $PSScriptRoot))){
        $candidateDll=Join-Path $repo ('build_x64/Release/'+$dllName)
        if((Test-Path -LiteralPath (Join-Path $repo 'CMakeLists.txt')) -and
           (Test-Path -LiteralPath $candidateDll -PathType Leaf)){
            $binary=(Resolve-Path -LiteralPath $candidateDll).Path
            $resourceRoot=Join-Path $repo 'data'
            break
        }
    }
}
if(!$binary){
    throw 'Falta la carpeta jcp-sur-lower-thirds con la DLL. Extraé TODO el ZIP junto al instalador; no ejecutes solamente el archivo .ps1. Si tenés los archivos en otra ubicación, usá -PluginRoot seguido de esa carpeta.'
}
if(!(Test-Path -LiteralPath (Join-Path $resourceRoot 'locale/es-ES.ini') -PathType Leaf)){
    throw 'Faltan los recursos del plugin (data/locale/es-ES.ini). Volvé a extraer el ZIP completo.'
}
if($ObsRoot){
    $root=(Resolve-Path -LiteralPath $ObsRoot).Path
    if(!(Test-Path -LiteralPath (Join-Path $root 'bin/64bit/obs64.exe') -PathType Leaf)){
        throw 'La carpeta elegida no contiene OBS Studio para Windows x64.'
    }
    $bin=Join-Path $root 'obs-plugins/64bit'
    $data=Join-Path $root 'data/obs-plugins/jcp-sur-lower-thirds'
}else{
    $destination=Join-Path $env:ProgramData 'obs-studio/plugins/jcp-sur-lower-thirds'
    $bin=Join-Path $destination 'bin/64bit'
    $data=Join-Path $destination 'data'
}
if($ValidateOnly){
    [pscustomobject]@{SourceDLL=$binary;SourceData=$resourceRoot;DestinationDLL=(Join-Path $bin $dllName);DestinationData=$data}
    return
}
if(Get-Process obs64 -ErrorAction SilentlyContinue){
    throw 'Cerrá OBS antes de instalar o actualizar el plugin. Después volvé a ejecutar este instalador.'
}
try{
    New-Item -ItemType Directory -Path $bin,$data -Force | Out-Null
    Copy-Item -LiteralPath $binary -Destination (Join-Path $bin $dllName) -Force
    foreach($item in Get-ChildItem -LiteralPath $resourceRoot){
        Copy-Item -LiteralPath $item.FullName -Destination $data -Recurse -Force
    }
}catch{
    throw ('No se pudo copiar el plugin a '+$bin+'. Si el error indica acceso denegado, abrí PowerShell como administrador. Detalle: '+$_.Exception.Message)
}
Write-Output 'Plugin instalado. Abrí OBS, activá Paneles > Zócalos y abrí Diseño con el engranaje.'
