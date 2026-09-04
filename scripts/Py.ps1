<#
  Py.ps1  -  Ejecuta un script Python de Unreal SIN interfaz (headless).
             La via principal de automatizacion "todo por codigo" del editor.
    .\scripts\Py.ps1 python\hello.py
    .\scripts\Py.ps1 python\build_greybox.py
    .\scripts\Py.ps1 -Code "import unreal; unreal.log('hola')"

  El script tiene acceso al modulo `unreal` completo (editor subsystems,
  spawn de actores, import de assets, edicion de niveles, cook parcial...).
#>
param(
    [Parameter(Position=0)] [string]$Script,
    [string]$Code,
    [switch]$KeepOpen,
    [switch]$NullRHI   # sin GPU/render: mas ligero y rapido (para scripts que no tocan viewport/thumbnails)
)
. "$PSScriptRoot\_env.ps1"

$common = @("`"$Uproject`"", '-stdout','-FullStdOutLogOutput','-unattended','-nopause','-nosplash')
if ($NullRHI)     { $common += @('-nullrhi','-NoShaderCompile') }
if (-not $KeepOpen) { $common += '-run=pythonscript' }

if ($Code) {
    $tmp = Join-Path $env:TEMP ("ue_py_" + [guid]::NewGuid().ToString('N') + ".py")
    Set-Content -Path $tmp -Value $Code -Encoding utf8
    $target = $tmp
} elseif ($Script) {
    $target = (Resolve-Path (Join-Path $ProjDir $Script) -ErrorAction SilentlyContinue).Path
    if (-not $target) { $target = (Resolve-Path $Script).Path }
} else {
    throw "Indica un fichero .py o -Code '<python>'"
}

# IMPORTANTE: barras normales. Con '\' el editor interpreta \t, \r, \b... como
# secuencias de escape y no encuentra el fichero.
$target = $target -replace '\\', '/'

Write-Head "Python headless: $target"
& $EditorCmd @common "-script=$target"
# OJO: no usar $code -> PowerShell no distingue mayusculas y pisaria el
# parametro -Code, dejando $tmp sin borrar y lanzando un error espurio.
$exitCode = $LASTEXITCODE
if ($Code -and $tmp -and (Test-Path $tmp)) { Remove-Item $tmp -Force }
Write-Host (">> salida $exitCode" ) -ForegroundColor $(if($exitCode -eq 0){'Green'}else{'Red'})
exit $exitCode
