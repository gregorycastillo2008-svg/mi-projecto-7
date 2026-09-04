# ============================================================================
#  _env.ps1  -  Entorno comun para todos los scripts de automatizacion UE5.
#  Dot-source:  . "$PSScriptRoot\_env.ps1"
# ============================================================================
$ErrorActionPreference = 'Stop'

# --- Motor -----------------------------------------------------------------
$Script:UERoot = $env:UNREAL_ROOT
if (-not $Script:UERoot -or -not (Test-Path $Script:UERoot)) {
    $Script:UERoot = 'C:\Program Files\Epic Games\UE_5.8\Engine'
}
if (-not (Test-Path $Script:UERoot)) {
    # fallback: primera UE_* que exista
    $cand = Get-ChildItem 'C:\Program Files\Epic Games\UE_*\Engine' -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($cand) { $Script:UERoot = $cand.FullName }
}
if (-not (Test-Path $Script:UERoot)) { throw "No encuentro el motor Unreal (UNREAL_ROOT / C:\Program Files\Epic Games\UE_5.8\Engine)." }

# --- Proyecto ------------------------------------------------------------
$Script:ProjDir  = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$uproj = Get-ChildItem $Script:ProjDir -Filter *.uproject | Select-Object -First 1
if (-not $uproj) { throw "No hay .uproject en $Script:ProjDir" }
$Script:Uproject = $uproj.FullName
$Script:ProjName = [IO.Path]::GetFileNameWithoutExtension($uproj.Name)

# --- Rutas de herramientas -------------------------------------------------
$Script:UBT        = Join-Path $UERoot 'Build\BatchFiles\Build.bat'
$Script:CleanBat   = Join-Path $UERoot 'Build\BatchFiles\Clean.bat'
$Script:RunUAT     = Join-Path $UERoot 'Build\BatchFiles\RunUAT.bat'
$Script:Editor     = Join-Path $UERoot 'Binaries\Win64\UnrealEditor.exe'
$Script:EditorCmd  = Join-Path $UERoot 'Binaries\Win64\UnrealEditor-Cmd.exe'

# --- dotnet embebido de UE ---------------------------------------------
$dotnetDir = Join-Path $UERoot 'Binaries\ThirdParty\DotNet\10.0\win-x64'
if (Test-Path $dotnetDir) {
    $env:PATH = "$dotnetDir;$env:PATH"
    $env:DOTNET_ROOT = $dotnetDir
    $env:DOTNET_MULTILEVEL_LOOKUP = '0'
    $env:DOTNET_ROLL_FORWARD = 'LatestMajor'
}

function Write-Head($t) { Write-Host "`n=== $t ===" -ForegroundColor Cyan }
function Assert-RAM($minFreeGB = 5) {
    $free = [math]::Round((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB,1)
    if ($free -lt $minFreeGB) {
        Write-Host "AVISO: solo $free GB de RAM libre. El editor UE5 quiere >$minFreeGB GB; cierra apps si va lento." -ForegroundColor Yellow
    } else {
        Write-Host "RAM libre: $free GB" -ForegroundColor DarkGray
    }
}

Write-Host "UE   : $UERoot" -ForegroundColor DarkGray
Write-Host "Proj : $Uproject ($ProjName)" -ForegroundColor DarkGray
