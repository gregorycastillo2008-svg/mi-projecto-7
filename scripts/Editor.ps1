<#
  Editor.ps1  -  Abre el editor con este proyecto.
    .\scripts\Editor.ps1            # abre el editor (compila antes si falta el .dll)
    .\scripts\Editor.ps1 -NoBuild
    .\scripts\Editor.ps1 -Wait     # no devuelve el prompt hasta cerrar el editor
#>
param([switch]$NoBuild, [switch]$Wait)
. "$PSScriptRoot\_env.ps1"
Assert-RAM 6

$dll = Join-Path $ProjDir "Binaries\Win64\UnrealEditor-$ProjName.dll"
if (-not $NoBuild -or -not (Test-Path $dll)) {
    & "$PSScriptRoot\Build.ps1" -Target Editor -Config Development
    if ($LASTEXITCODE -ne 0) { throw "Build fallo; no lanzo el editor." }
}

Write-Head "Abriendo editor"
Write-Host "   $Editor `"$Uproject`"" -ForegroundColor DarkGray
$p = Start-Process $Editor -ArgumentList "`"$Uproject`"" -PassThru
if ($Wait) { $p.WaitForExit(); Write-Host ">> editor cerrado (exit $($p.ExitCode))" }
else { Write-Host ">> editor lanzado (pid $($p.Id)). 1er arranque compila shaders: puede tardar." -ForegroundColor Green }
