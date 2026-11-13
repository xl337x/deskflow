<#
  Install this build of Deskflow (with DeskView) over the installed Deskflow, for testing.
  Needs administrator rights (asks once). Keeps a copy of the original in "C:\Program Files\Deskflow.orig".

    powershell -ExecutionPolicy Bypass -File deskview-deploy.ps1            deploy build\bin
    powershell -ExecutionPolicy Bypass -File deskview-deploy.ps1 -Rollback  put the original back
#>
param([switch]$Rollback, [switch]$Elevated)
$ErrorActionPreference = 'Stop'
$target = 'C:\Program Files\Deskflow'
$backup = 'C:\Program Files\Deskflow.orig'
$source = Join-Path $PSScriptRoot 'build\bin'

if (-not $Elevated) {
    $a = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -Elevated" + $(if ($Rollback) { ' -Rollback' } else { '' })
    $p = Start-Process powershell -Verb RunAs -Wait -PassThru -ArgumentList $a
    if ($p.ExitCode -ne 0) { throw "Deploy failed (exit $($p.ExitCode)) - see $env:TEMP\deskview-deploy.log" }
    # The GUI runs as the signed-in user, not elevated.
    Start-Process (Join-Path $target 'deskflow.exe') -WindowStyle Minimized
    Get-Content "$env:TEMP\deskview-deploy.log" -Tail 5
    exit 0
}

Start-Transcript "$env:TEMP\deskview-deploy.log" -Force | Out-Null
try {
    if (-not $Rollback -and -not (Test-Path $backup)) { Copy-Item $target $backup -Recurse }
    Stop-Service Deskflow -Force
    Get-Process deskflow, deskflow-core -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep 1
    if ($Rollback) {
        if (-not (Test-Path $backup)) { throw "No backup at $backup" }
        robocopy $backup $target /MIR /NFL /NDL /NJH /NP | Out-Null
        Write-Host 'Original Deskflow restored.'
    } else {
        robocopy $source $target /E /NFL /NDL /NJH /NP | Out-Null
        Set-Content (Join-Path $target 'deskview.txt') "DeskView build $(Get-Date -Format s): native portal" -Encoding ASCII
        Write-Host "Deployed $source"
    }
    Start-Service Deskflow
    Write-Host 'Deskflow service restarted.'
    exit 0
} catch { Write-Host "ERROR: $_"; exit 1 }
finally { Stop-Transcript | Out-Null }
