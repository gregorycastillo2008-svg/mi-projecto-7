<#
  GenerateProjectFiles.ps1  -  Regenera .code-workspace + .vscode/ + compileCommands.
  Ejecutar tras: añadir/quitar .cpp/.h, cambiar *.Build.cs, o un Clean -Hard.
#>
. "$PSScriptRoot\_env.ps1"
Write-Head "GenerateProjectFiles (VSCode)"
& $UBT -projectfiles -project="$Uproject" -game -engine -progress -VSCode
if ($LASTEXITCODE -eq 0) {
    Write-Host "`n>> OK. Abre en VS Code:  $ProjDir\$ProjName.code-workspace" -ForegroundColor Green
} else {
    Write-Host "`n>> FALLO (codigo $LASTEXITCODE)" -ForegroundColor Red
}
exit $LASTEXITCODE
