param(
    [switch]$NoBuild,
    [int]$TimeoutSeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "TestWorkflow.ps1")

$generateExit = Invoke-GenerateOutputs -NoBuild:$NoBuild -TimeoutSeconds $TimeoutSeconds
exit $generateExit
