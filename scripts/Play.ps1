<#
  Play.ps1  -  Lanza el JUEGO (sin editor), en ventana, con un mapa.
    .\scripts\Play.ps1
    .\scripts\Play.ps1 -Map /Game/FirstPerson/Lvl_FirstPerson
    .\scripts\Play.ps1 -WindowSize 1600x900 -NoBuild
  Corre el proyecto con UnrealEditor-Cmd -game (usa el codigo del editor, no
  necesita cocinar). Para el .exe empaquetado usa Package.ps1.
#>
param(
    [string]$Map = '',
    [string]$WindowSize = '1600x900',
    [switch]$NoBuild
)
. "$PSScriptRoot\_env.ps1"
Assert-RAM 5

if (-not $NoBuild) {
    & "$PSScriptRoot\Build.ps1" -Target Editor -Config Development
    if ($LASTEXITCODE -ne 0) { throw "Build fallo." }
}

$w,$h = $WindowSize -split 'x'
$args = @("`"$Uproject`"")
if ($Map) { $args += $Map }
$args += @('-game','-windowed',"-ResX=$w","-ResY=$h",'-nosplash')

Write-Head "Play"
Write-Host "   $EditorCmd $($args -join ' ')" -ForegroundColor DarkGray
$p = Start-Process $EditorCmd -ArgumentList $args -PassThru
Write-Host ">> juego lanzado (pid $($p.Id)). ESC/Alt+F4 para cerrar." -ForegroundColor Green
