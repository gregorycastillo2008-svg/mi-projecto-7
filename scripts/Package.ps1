<#
  Package.ps1 - crea un EXE de Windows listo para ejecutar fuera de Unreal.
  El resultado queda en Builds\WindowsShipping\Windows\MyProject7.exe.
#>
param(
    [ValidateSet('Development','Shipping')]
    [string]$Config = 'Shipping'
)

. "$PSScriptRoot\_env.ps1"

$OutputDir = Join-Path $ProjDir "Builds\Windows$Config"
$UatLog = Join-Path $env:TEMP ("uat_package_" + $ProjName + ".log")

Write-Head "EMPAQUETAR EXE  Win64 / $Config"
Write-Host "Salida: $OutputDir" -ForegroundColor DarkGray

$Arguments = @(
    'BuildCookRun',
    "-project=$Uproject",
    '-noP4',
    '-platform=Win64',
    ("-clientconfig={0}" -f $Config),
    '-build', '-nocompileeditor', '-cook', '-stage', '-pak', '-archive',
    "-archivedirectory=$OutputDir",
    '-utf8output', '-unattended', '-NoUBA',
    "-log=$UatLog"
)

& $RunUAT @Arguments

exit $LASTEXITCODE
