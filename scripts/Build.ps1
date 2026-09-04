<#
  Build.ps1  -  Compila el modulo C++ del proyecto con UnrealBuildTool.
  Uso:
    .\scripts\Build.ps1                       # Editor / Development (lo normal en desarrollo)
    .\scripts\Build.ps1 -Target Game          # el ejecutable del juego, no el editor
    .\scripts\Build.ps1 -Config DebugGame     # game code debuggable, engine en Development
    .\scripts\Build.ps1 -Config Shipping -Target Game
#>
param(
    [ValidateSet('Editor','Game')]           [string]$Target = 'Editor',
    [ValidateSet('Development','DebugGame','Debug','Shipping','Test')] [string]$Config = 'Development'
)
. "$PSScriptRoot\_env.ps1"

$targetName = if ($Target -eq 'Editor') { "$ProjName`Editor" } else { $ProjName }
Write-Head "UBT  $targetName  Win64  $Config"

$sw = [Diagnostics.Stopwatch]::StartNew()
# -NoUBA : sin esto el Unreal Build Accelerator puede quedarse colgado 20-30 min.
# -Log   : UBT escribe en UnrealBuildTool\Log.txt, que VS Code / otros agentes
#          suelen tener abierto y hace fallar la compilacion con IOException.
$ubtLog = Join-Path $env:TEMP ("ubt_" + $ProjName + ".txt")
& $UBT $targetName Win64 $Config -project="$Uproject" -waitmutex -NoHotReload -NoUBA -Log="$ubtLog"
$code = $LASTEXITCODE
$sw.Stop()

if ($code -eq 0) {
    Write-Host ("`n>> BUILD OK  ({0})  {1:mm\:ss}" -f $targetName, $sw.Elapsed) -ForegroundColor Green
} else {
    Write-Host "`n>> BUILD FALLO (codigo $code)" -ForegroundColor Red
}
exit $code
