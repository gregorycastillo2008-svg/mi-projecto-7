<#  Rebuild.ps1  -  Clean + Build del target indicado.  #>
param(
    [ValidateSet('Editor','Game')] [string]$Target = 'Editor',
    [ValidateSet('Development','DebugGame','Debug','Shipping','Test')] [string]$Config = 'Development'
)
. "$PSScriptRoot\_env.ps1"
$targetName = if ($Target -eq 'Editor') { "$ProjName`Editor" } else { $ProjName }

Write-Head "CLEAN  $targetName  Win64  $Config"
& $CleanBat $targetName Win64 $Config -project="$Uproject" -waitmutex

& "$PSScriptRoot\Build.ps1" -Target $Target -Config $Config
exit $LASTEXITCODE
